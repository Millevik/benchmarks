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

using increment_atom = atom_constant<atom("increment")>;
using retreive_atom = atom_constant<atom("retreive")>;
using result_atom = atom_constant<atom("result")>;

behavior produce_actor(event_based_actor* self, int count, actor counting) {
  return {
    [=](increment_atom) {
      for (int i = 0; i < count; ++i) {
        self->send(counting, increment_atom::value);
      }
      self->send(counting, retreive_atom::value);
    },
    [=](result_atom, int result) {
      if (result != count) {
        cout << "ERROR: expected: " << count << ", found: " << result << endl;
      } else {
        cout << "SUCCESS! received: " << result << endl;
      }
    }
  };
}

behavior counting_actor(stateful_actor<int>* self) {
  self->state = 0;
  return {
    [=](increment_atom) { 
      ++self->state; 
    },
    [=](retreive_atom) -> result<result_atom, int> {
      return {result_atom::value, self->state};
    }
  };
}

class config : public actor_system_config {
public:
  int n = 1e6;

  config() {
    opt_group{custom_options_, "global"}.add(n, "num,n", "number of messages");
  }
};

void caf_main(actor_system& system, const config& cfg) {
  auto counting = system.spawn(counting_actor);
  auto produce = system.spawn(produce_actor, cfg.n, counting);
  anon_send(produce, increment_atom::value);
}

CAF_MAIN()
