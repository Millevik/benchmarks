/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2016                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENCE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#include <iostream>
#include <vector>

#include "caf/all.hpp"

using namespace std;
using namespace caf;

using work_atom = atom_constant<atom("work")>;
using result_atom = atom_constant<atom("result")>;
using done_atom = atom_constant<atom("done")>;
using stop_atom = atom_constant<atom("stop")>;

class config : public actor_system_config {
public:
  int num_workers = 20;
  int size = 12;
  int threshold = 4;
  int priorities = 10;
  int solutions_limit = 1500000;

  config() {
    opt_group{custom_options_, "global"}
      .add(num_workers, "workers", "number of workers")
      .add(size, "size", "size of the chess board")
      .add(threshold, "threshold",
           "threshold for switching from parrallel processing to sequential "
           "processing")
      .add(solutions_limit, "solutions-limit", "solutions limit???");
  }
};

struct work_msg {
  int priority;
  vector<int> data;
  int depth;
};

#define CAF_ALLOW_UNSAFE_MESSAGE_TYPE(type_name)                               \
  namespace caf {                                                              \
  template <>                                                                  \
  struct allowed_unsafe_message_type<type_name> : std::true_type {};           \
  }

CAF_ALLOW_UNSAFE_MESSAGE_TYPE(work_msg);

// a contains array of n queen positions. Returns 1
// if none of the queens conflict, and returns 0 otherwise.
bool board_valid(int n, const vector<int>& a) {
  for (int i = 0; i < n; i++) {
    int p = a[i];
    for (int j = (i + 1); j < n; j++) {
      int q = a[j];
      if (q == p || q == p - (j - i) || q == p + (j - i))
        return false;
    }
  }
  return true;
};

// calc N-Queens problem sequential
// recusive function cannot be defined as lambda function
void nqueens_kernel_seq(event_based_actor* self, actor master, int size,
                        vector<int> a, int depth) {
  if (size == depth)
    self->send(master, result_atom::value);
  else {
    for (int i = 0; i < size; ++i) {
      auto b = a;
      b.emplace_back(i);
      if (board_valid(depth + 1, b)) {
        nqueens_kernel_seq(self, master, size, b, depth + 1);
      }
    }
  }
};

behavior worker_actor(event_based_actor* self, actor master,
                      const config* cfg) {
  auto size = cfg->size;
  auto threshold = cfg->threshold;
  // calc N-Queens problem in parallel
  auto nqueens_kernel_par = [=](work_msg&& msg) {
    auto& a = msg.data;
    auto& depth = msg.depth;
    if (size == depth)
      self->send(master, result_atom::value);
    else if (depth >= threshold)
      nqueens_kernel_seq(self, master, size, a, depth);
    else {
      auto new_priority = msg.priority - 1;
      auto new_depth = depth + 1;
      for (int i = 0; i < size; ++i) {
        auto b = a;
        b.emplace_back(i);
        if (board_valid(new_depth, b)) {
          self->send(master, work_atom::value,
                     work_msg{new_priority, move(b), new_depth});
        }
      }
    }
  };

  return {[=](work_atom, work_msg msg) {
            nqueens_kernel_par(move(msg));
            self->send(master, done_atom::value);
          },
          [=](stop_atom) {
            self->send(master, stop_atom::value);
            self->quit();
          }};
}

struct master_data {
  vector<actor> workers;
  int message_counter = 0;
  int num_worker_send = 0;
  long result_counter = 0;
  int num_work_completed = 0;
  int num_workers_terminated = 0;
};

behavior master_actor(stateful_actor<master_data>* self, actor result,
                      const config* cfg) {
  auto num_workers = cfg->num_workers;
  auto priorities = cfg->priorities;
  auto solutions_limit = cfg->solutions_limit;
  auto send_work = [=](work_msg&& msg) {
    self->send(self->state.workers[self->state.message_counter],
               work_atom::value, move(msg));
    self->state.message_counter =
      (self->state.message_counter + 1) % num_workers;
    ++self->state.num_worker_send;
  };
  auto request_workers_to_terminate = [=]() {
    for (auto e : self->state.workers) {
      self->send(e, stop_atom::value);
    }
  };
  self->state.workers.reserve(num_workers);
  for (int i = 0; i < num_workers; ++i) {
    self->state.workers.emplace_back(
      self->spawn(worker_actor, actor_cast<actor>(self), cfg));
  }
  send_work({priorities, {}, 0});
  return {[=](work_atom, work_msg msg) { send_work(move(msg)); },
          [=](result_atom) {
            ++self->state.result_counter;
            if (self->state.result_counter == solutions_limit) {
              request_workers_to_terminate();
            }
          },
          [=](done_atom) {
            ++self->state.num_work_completed;
            if (self->state.num_work_completed == self->state.num_worker_send) {
              request_workers_to_terminate();
            }
          },
          [=](stop_atom) {
            ++self->state.num_workers_terminated;
            if (self->state.num_workers_terminated == num_workers) {
              self->send(result, self->state.result_counter);
              self->quit();
            }
          }};
}

void caf_main(actor_system& system, const config& cfg) {
  scoped_actor self{system};
  system.spawn(master_actor, self, &cfg);

  long act_solution;
  bool valid;
  self->receive([&](long result_count) {
    act_solution = result_count;
    valid = act_solution >= cfg.solutions_limit;
    cout << "act_solution: " << act_solution << " limit:" << cfg.solutions_limit
         << endl;
  });
  cout << "Solutions found:" << act_solution << endl;
  cout << "Result valid:" << valid << endl; // bug in savina???
}

CAF_MAIN()
