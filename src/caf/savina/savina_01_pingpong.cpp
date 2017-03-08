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

#include "caf/all.hpp"

using namespace std;
using namespace caf;

using start_atom = atom_constant<atom("start")>;
using ping_atom = atom_constant<atom("ping")>;
using pong_atom = atom_constant<atom("pong")>;
using stop_atom = atom_constant<atom("stop")>;

behavior ping_actor(stateful_actor<int>* self, int count, actor pong) {
  self->state = count;
  return {
    [=](start_atom) {
        self->send(pong, ping_atom::value);
        --self->state;
      },
      [=](ping_atom) {
        self->send(pong, ping_atom::value);
        --self->state;
      },
      [=](pong_atom) {
        if (self->state > 0) {
          self->send(self, ping_atom::value);
        } else {
          self->send(pong, stop_atom::value);
          self->quit();
        }
    }
  };
}

behavior pong_actor(stateful_actor<int>* self) {
  self->state = 0;
  return {
    [=](ping_atom) { 
      ++self->state;
      return pong_atom::value; 
    },
    [=](stop_atom) { 
      self->quit(); 
    }
  };
}

class config : public actor_system_config {
public:
  int n = 40000;

  config() {
    opt_group{custom_options_, "global"}.add(n, "num,n", "number of ping-pongs");
  }
};

void caf_main(actor_system& system, const config& cfg) {
  auto pong = system.spawn(pong_actor);
  auto ping = system.spawn(ping_actor, cfg.n, pong);
  anon_send(ping, start_atom::value);
}

CAF_MAIN()
