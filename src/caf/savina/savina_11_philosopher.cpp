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

using start_atom = atom_constant<atom("start")>;
using exit_atom = atom_constant<atom("exit")>;
using hungry_atom = atom_constant<atom("hungry")>;
using done_atom = atom_constant<atom("done")>;
using eat_atom = atom_constant<atom("eat")>;
using denied_atom = atom_constant<atom("denied")>;

class config : public actor_system_config {
public:
  int num_philosophers = 20;
  int num_eating_rounds = 10000;
  int num_channels = 1;

  config() {
    opt_group{custom_options_, "global"}
      .add(num_philosophers, "philosophers", "number of philosophers")
      .add(num_eating_rounds, "rounds", "number of eating rounds")
      .add(num_channels, "channles", "number of channels");
  }
};

struct philosopher_states {
  long local_counter = 0;
  long counter = 0;
  int rounds_so_far = 0;
};

behavior philosopher_actor(stateful_actor<philosopher_states>* self, int id,
                           actor arbitrator, strong_actor_ptr result,
                           const config* cfg) {
  return {[=](denied_atom) {
            ++self->state.local_counter;
            self->send(arbitrator, hungry_atom::value, id);
          },
          [=](eat_atom) {
            ++self->state.rounds_so_far;
            self->state.counter += self->state.local_counter;
            self->send(arbitrator, done_atom::value, id);
            if (self->state.rounds_so_far < cfg->num_eating_rounds) {
              self->send(self, start_atom::value);
            } else {
              self->send(arbitrator, exit_atom::value);
              self->send(actor_cast<actor>(result), self->state.counter);
              // self->quit()
            }
          },
          [=](start_atom) { self->send(arbitrator, hungry_atom::value, id); }};
}

behavior arbitrator_actor(stateful_actor<vector<int>>* self,
                          const config* cfg) {
  self->state.reserve(cfg->num_philosophers);
  for (int i = 0; i < cfg->num_philosophers; ++i) {
    self->state.emplace_back(0); // fork not used
  }
  int num_exit_philosophers = 0;
  return {[=](hungry_atom, int id) {
            int& left_fork = self->state[id];
            int& right_fork = self->state[(id + 1) % self->state.size()];

            if (left_fork || right_fork) {
              self->send(actor_cast<actor>(self->current_sender()),
                         denied_atom::value);
            } else {
              left_fork = 1;
              right_fork = 1;
              self->send(actor_cast<actor>(self->current_sender()),
                         eat_atom::value);
            }
          },
          [=](done_atom, int id) {
            int& left_fork = self->state[id];
            int& right_fork = self->state[(id + 1) % self->state.size()];
            left_fork = 0;
            right_fork = 0;
          },
          [=](exit_atom) mutable {
            ++num_exit_philosophers;
            if (cfg->num_philosophers == num_exit_philosophers) {
              // self->quit()
            }
          }};
}

void caf_main(actor_system& system, const config& cfg) {
  auto arbitrator = system.spawn(arbitrator_actor, &cfg);
  scoped_actor self{system};
  for (int i = 0; i < cfg.num_philosophers; ++i) {
    auto philosopher = system.spawn(philosopher_actor, i, arbitrator,
                                    actor_cast<strong_actor_ptr>(self), &cfg);
    self->send(philosopher, start_atom::value);
  }
  long counter = 0;
  for (int i = 0; i < cfg.num_philosophers; ++i) {
    self->receive([&](long x) { counter += x; });
  }
  cout << " Num retries: " << counter << std::endl;
}

CAF_MAIN()
