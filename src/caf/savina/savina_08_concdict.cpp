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
#include <random>
#include <unordered_map>

#include "caf/all.hpp"

using namespace std;
using namespace caf;

using write_atom = atom_constant<atom("write")>;
using read_atom = atom_constant<atom("read")>;
using end_atom = atom_constant<atom("end")>;
using do_work_atom = atom_constant<atom("do_work")>;

class config : public actor_system_config {
public:
  int num_workers = 20;
  int num_msgs_per_worker = 10000;
  int write_percentage = 10;

  config() {
    opt_group{custom_options_, "global"}
      .add(num_workers, "worker", "number of workers")
      .add(num_msgs_per_worker, "msgs_per_worker",
           "number of messges per worker")
      .add(write_percentage, "wpercent", "write percent");
  }
};

behavior dictionary_actor(stateful_actor<unordered_map<int, int>>* self) {
  return {[=](write_atom, int key, int value) -> result<do_work_atom, int> {
            self->state[key] = value;
            return {do_work_atom::value, value};
          },
          [=](read_atom, int key) -> result<do_work_atom, int> {
            return {do_work_atom::value, self->state[key]};
          },
          [=](end_atom) {
            cout << "Dictionary Size: " << self->state.size() << endl;
            // self->quit();
          }};
}

behavior worker_actor(event_based_actor* self, actor master, actor dictionary,
                      int id, const config* cfg) {
  int message_count = 0;
  std::default_random_engine rengine(id + cfg->num_msgs_per_worker
                                     + cfg->write_percentage);
  std::uniform_int_distribution<int> uniform(0, 99);
  return {[=](do_work_atom, int /*result*/) mutable {
    ++message_count;
    if (message_count <= cfg->num_msgs_per_worker) {
      if (uniform(rengine) < cfg->write_percentage) {
        self->send(dictionary, write_atom::value, uniform(rengine),
                   uniform(rengine));
      } else {
        self->send(dictionary, read_atom::value, uniform(rengine));
      }
    } else {
      self->send(master, end_atom::value);
      // self->quit();
    }
  }};
}

behavior master_actor(event_based_actor* self, const config* cfg) {

  auto dictionary = self->spawn(dictionary_actor);
  for (int i = 0; i < cfg->num_workers; ++i) {
    auto worker =
      self->spawn(worker_actor, actor_cast<actor>(self), dictionary, i, cfg);
    self->send(worker, do_work_atom::value, 0);
  }
  int num_worker_terminated = 0;

  return {
    [=](end_atom) mutable {
      ++num_worker_terminated;
      if (num_worker_terminated == cfg->num_workers) {
        self->send(dictionary, end_atom::value);
      }
      // self->quit();
    },
  };
}

void caf_main(actor_system& system, const config& cfg) {
  auto master = system.spawn(master_actor, &cfg);
}

CAF_MAIN()
