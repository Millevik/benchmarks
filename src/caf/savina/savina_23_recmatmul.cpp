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
#include <algorithm>
#include <map>

#include "caf/all.hpp"

using namespace std;
using std::chrono::seconds;
using namespace caf;

#define CAF_ALLOW_UNSAFE_MESSAGE_TYPE(type_name)                               \
  namespace caf {                                                              \
  template <>                                                                  \
  struct allowed_unsafe_message_type<type_name> : std::true_type {};           \
  }

template<class T>
struct matrix_t {
  matrix_t(size_t y, size_t x) 
      : width_(x)
      , v_(y * x){
    // nop 
  }
  matrix_t() = default;
  matrix_t(const matrix_t&) = default;
  matrix_t(matrix_t&&) = default;
  matrix_t& operator=(const matrix_t&) = default;
  matrix_t& operator=(matrix_t&&) = default;
  
  inline T& operator()(size_t y, size_t x) {
    return v_[y * width_ + x];
  };
private:
  size_t width_;
  vector<T> v_;
};


class config : public actor_system_config {
public:
  static int num_workers; // = 20;
  static int data_length; // = 1024;
  static int block_threshold; //= 16384;

  static matrix_t<double> a;
  static matrix_t<double> b;
  static matrix_t<double> c;
  config() {
    opt_group{custom_options_, "global"}
      .add(data_length, "nnn,n", "data length")
      .add(block_threshold, "ttt,t", "block_trheshold")
      .add(num_workers, "www,w", "number of workers");
  }

  void initialize_data() const {
    a = matrix_t<double>(data_length, data_length);
    b = matrix_t<double>(data_length, data_length);
    c = matrix_t<double>(data_length, data_length);
    
    for (int i = 0; i < data_length; i++) {
      for (int j = 0; j < data_length; j++) {
        a(i, j) = i;
        b(i, j) = j;
      }
    }
  }

  bool valid() const {
    for (int i = 0; i < data_length; i++) {
      for (int j = 0; j < data_length; j++) {
        double actual = c(i, j);
        double expected = 1.0 * data_length * i * j;
        if (compare(actual, expected) != 0) {
          return false;
        }
      }
    }
    return true;
  }

  int compare(double d1, double d2) const {
    // ignore NAN and other double problems
    // probably faster than the java doubel compare implemenation
    if (d1 < d2)
        return -1;
    if (d1 > d2)
        return 1;
    return 0;
  }
};
matrix_t<double> config::a;
matrix_t<double> config::b;
matrix_t<double> config::c;
int config::num_workers = 20;
int config::data_length = 1024;
int config::block_threshold = 16384;

struct work_msg {
  int priority;
  int srA;
  int scA;
  int srB;
  int scB;
  int srC;
  int scC;
  int numBlocks;
  int dim;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(work_msg);

using done_msg_atom = atom_constant<atom("done")>;
using stop_msg_atom = atom_constant<atom("stop")>;

behavior worker_fun(event_based_actor* self, actor master, int /*id*/) {
  int threshold = config::block_threshold;
  auto my_rec_mat = [=](work_msg& work_message) {
    int srA = work_message.srA;
    int scA = work_message.scA;
    int srB = work_message.srB;
    int scB = work_message.scB;
    int srC = work_message.srC;
    int scC = work_message.scC;
    int numBlocks = work_message.numBlocks;
    int dim = work_message.dim;
    int newPriority = work_message.priority + 1;
    if (numBlocks > threshold) {
      int zerDim = 0;
      int newDim = dim / 2;
      int newNumBlocks = numBlocks / 4;
      self->send(master, work_msg{newPriority, srA + zerDim, scA + zerDim, srB + zerDim, scB + zerDim, srC + zerDim, scC + zerDim, newNumBlocks, newDim});
      self->send(master, work_msg{newPriority, srA + zerDim, scA + newDim, srB + newDim, scB + zerDim, srC + zerDim, scC + zerDim, newNumBlocks, newDim});
      self->send(master, work_msg{newPriority, srA + zerDim, scA + zerDim, srB + zerDim, scB + newDim, srC + zerDim, scC + newDim, newNumBlocks, newDim});
      self->send(master, work_msg{newPriority, srA + zerDim, scA + newDim, srB + newDim, scB + newDim, srC + zerDim, scC + newDim, newNumBlocks, newDim});
      self->send(master, work_msg{newPriority, srA + newDim, scA + zerDim, srB + zerDim, scB + zerDim, srC + newDim, scC + zerDim, newNumBlocks, newDim});
      self->send(master, work_msg{newPriority, srA + newDim, scA + newDim, srB + newDim, scB + zerDim, srC + newDim, scC + zerDim, newNumBlocks, newDim});
      self->send(master, work_msg{newPriority, srA + newDim, scA + zerDim, srB + zerDim, scB + newDim, srC + newDim, scC + newDim, newNumBlocks, newDim});
      self->send(master , work_msg{newPriority, srA + newDim, scA + newDim, srB + newDim, scB + newDim, srC + newDim, scC + newDim, newNumBlocks, newDim});
    } else {
      aout(self) << "XXX" << endl;
      auto& a = config::a;
      auto& b = config::b;
      auto& c = config::c;
      int endR = srC + dim;
      int endC = scC + dim;
      int i = srC;
      while (i < endR) {
        int j = scC;
        while (j < endC) {
          {
            int k = 0;
            while (k < dim) {
              c(i, j) += a(i, scA + k) * b(srB + k, j);
              k += 1;
            }
          }
          j += 1;
        }
        i += 1;
      }
    }
  };
  return  {
    [=](work_msg& work_message) {
      my_rec_mat(work_message);
      self->send(master, done_msg_atom::value);
    },
    [=](stop_msg_atom the_msg) {
      self->send(master, the_msg);
      self->quit();
    },
  };
}

struct master_state {
  int num_workers;
  vector<actor> workers;
  int num_workers_terminated;
  int num_work_sent;
  int num_work_completed;
};

behavior master_fun(stateful_actor<master_state>* self) {
  auto& s = self->state;
  s.num_workers = config::num_workers;
  s.workers.reserve(s.num_workers);
  s.num_workers_terminated = 0;
  s.num_work_sent = 0;
  s.num_work_completed = 0;
  auto send_work = [=](work_msg&& work_message) {
    auto& s = self->state;
    int work_index = (work_message.srC + work_message.scC) % s.num_workers; 
    self->send(s.workers[work_index], move(work_message));
    ++s.num_work_sent;
  };
  // onPostStart()
  {
    for (int i = 0; i < s.num_workers; ++i) {
      s.workers.emplace_back(
        self->spawn(worker_fun, actor_cast<actor>(self), i));
    }
    int data_length = config::data_length;
    int num_blocks = config::data_length * config::data_length;
    send_work(work_msg{0, 0, 0, 0, 0, 0, 0, num_blocks, data_length});
  }
  return {
    [=](work_msg& work_message) {
      send_work(move(work_message));
    },
    [=](done_msg_atom) {
      auto& s = self->state;
      ++s.num_work_completed;
      if (s.num_work_completed == s.num_work_sent) {
        for (int i = 0; i < s.num_workers; ++i) {
          self->send(s.workers[i], stop_msg_atom::value);
        } 
      }
    },
    [=](stop_msg_atom) {
      auto& s = self->state;
      ++s.num_workers_terminated;
      if (s.num_workers_terminated == s.num_workers) {
        self->quit(); 
      }
    }
  };
}

void caf_main(actor_system& system, const config& cfg) {
  cfg.initialize_data();
  auto master = system.spawn(master_fun);
  system.await_all_actors_done(); 
  bool is_valid = cfg.valid();
  cout << "Result valid: " << is_valid << endl;
}

CAF_MAIN()
