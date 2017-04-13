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
#include <atomic>
#include <deque>

#include "caf/all.hpp"

#include "savina_helper.hpp"

using namespace std;
using namespace caf;

#define CAF_ALLOW_UNSAFE_MESSAGE_TYPE(type_name)                               \
  namespace caf {                                                              \
  template <>                                                                  \
  struct allowed_unsafe_message_type<type_name> : std::true_type {};           \
  }

class config : public actor_system_config {
public:
  int r = 1000;
  int s = 200;

  config() {
    opt_group{custom_options_, "global"}
      .add(r, "rrr,r", "number of rounds")
      .add(s, "sss,s", "num smokers, ingredients");
  }
};

int busy_wait(int limit) {
  int test = 0;
  for (int k = 0; k < limit; ++k) {
      drand48();
      ++test; 
  }
  return test;
}

struct start_smoking {
  int busy_wait_period;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(start_smoking);

using started_smoking_atom = atom_constant<atom("started")>;
using start_msg_atom = atom_constant<atom("startmsg")>;
using exit_msg_atom = atom_constant<atom("exitmsg")>;

behavior smoker_actor_fun(event_based_actor* self, actor arbiter_actor) {
  return {
    [=](start_smoking& sm) {
      self->send(arbiter_actor, started_smoking_atom::value); 
      busy_wait(sm.busy_wait_period);
    },
    [=](exit_msg_atom) {
      self->quit(); 
    } 
  };
}

struct arbiter_actor_state {
  vector<actor> smoker_actors;
  random_gen random;
  int rounds_so_far;
};

behavior arbiter_actor_fun(stateful_actor<arbiter_actor_state>* self,
                           int num_rounds, int num_smokers) {
  auto& s = self->state;
  s.smoker_actors.reserve(num_smokers);
  auto my_self = actor_cast<actor>(self);
  for (int i = 0; i < num_smokers; ++i) {
    s.smoker_actors.emplace_back(self->spawn(smoker_actor_fun, my_self));
  }
  s.random.set_seed(num_rounds * num_smokers);
  s.rounds_so_far = 0;
  auto notify_random_smoker = [=]() {
    auto& s = self->state; 
    auto new_smoker_index = abs(s.random.next_int()) % num_smokers;
    auto busy_wait_period = s.random.next_int(1000) + 10;
    self->send(s.smoker_actors[new_smoker_index], start_smoking{busy_wait_period});
  };
  auto request_smokers_to_exit = [=]() {
    for (auto& loop_actor : self->state.smoker_actors) {
      self->send(loop_actor, exit_msg_atom::value); 
    } 
  };
  return {
    [=](start_msg_atom) {
      notify_random_smoker(); 
    },
    [=](started_smoking_atom) {
      auto& s = self->state;
      ++s.rounds_so_far;
      if (s.rounds_so_far >= num_rounds) {
        request_smokers_to_exit(); 
        self->quit();
      } else {
        notify_random_smoker();
      }
    }
  };
}

void caf_main(actor_system& system, const config& cfg) {
  auto arbiter_actor = system.spawn(arbiter_actor_fun, cfg.r, cfg.s);
  anon_send(arbiter_actor, start_msg_atom::value);
}

CAF_MAIN()
