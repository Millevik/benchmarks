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

using namespace std;
using namespace caf;

#define CAF_ALLOW_UNSAFE_MESSAGE_TYPE(type_name)                               \
  namespace caf {                                                              \
  template <>                                                                  \
  struct allowed_unsafe_message_type<type_name> : std::true_type {};           \
  }

class config : public actor_system_config {
public:
  int n = 5000;
  int w = 1000;
  static int apr; // = 1000;
  static int ahr; // = 1000;

  config() {
    opt_group{custom_options_, "global"}
      .add(n, "nnn,n", "nmber of haircuts")
      .add(w, "www,w", "waiting room size")
      .add(apr, "ppp,p", "average producer rate")
      .add(ahr, "ccc,c", "average haircut rate");
  }
};
int config::ahr = 1000;
int config::apr = 1000;

int busy_wait(int limit) {
  int test = 0;
  for (int k = 0; k < limit; ++k) {
      drand48();
      ++test; 
  }
  return test;
}

using full_atom = atom_constant<atom("full")>;
using wait_atom = atom_constant<atom("wait")>;
using next_atom = atom_constant<atom("next")>;
using start_atom = atom_constant<atom("start")>;
using done_atom = atom_constant<atom("done")>;
using exit_atom = atom_constant<atom("exit")>;

struct enter {
  actor customer;
  actor room;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(enter);

struct returned {
  actor customer;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(returned);

struct waiting_room_actor {
  deque<actor> waiting_customers;
  bool barber_asleep = true;
};

int next_int(default_random_engine& r,
             int inclusive_max = std::numeric_limits<int>::max()) {
  return r() % (inclusive_max + 1);
};

behavior waiting_room_actor(stateful_actor<waiting_room_actor>* self, size_t capacity,
                            actor barber) {
  return {
    [=](enter& msg) {
      auto& s = self->state;
      auto& customer = msg.customer; 
      if (s.waiting_customers.size() == capacity) {
        self->send(customer, full_atom::value); 
      } else {
        s.waiting_customers.emplace_back(customer);
        if (s.barber_asleep) {
          s.barber_asleep = false;
          self->send(actor_cast<actor>(self), next_atom::value);
        } else {
          self->send(customer, wait_atom::value); 
        }
      }
    },
    [=](next_atom) {
      auto& s = self->state;
      if (s.waiting_customers.size() > 0) {
        auto& customer = s.waiting_customers.front(); 
        self->send(barber, enter{move(customer), actor_cast<actor>(self)});
        s.waiting_customers.pop_front();
      } else {
        self->send(barber, wait_atom::value);
        s.barber_asleep = true;
      }
    },
    [=](exit_atom) {
      self->send(barber, exit_atom::value);
      self->quit();
    }
  };
}

behavior barber_actor(event_based_actor* self) {
  default_random_engine random{};
  return {
    [=](enter& msg) mutable {
      auto& customer = msg.customer;
      auto& room = msg.room;
      self->send(customer, start_atom::value);
      busy_wait(next_int(random, config::ahr) + 10);
      self->send(customer, done_atom::value);
      self->send(room, next_atom::value);
    },
    [=](wait_atom) {
      // nop
    },
    [=](exit_atom) {
      self->quit();
    }
  };
}

behavior customer_actor(event_based_actor* self, long /*id*/, actor factory_actor) {
  return {
    [=](full_atom) {
      self->send(factory_actor, returned{actor_cast<actor>(self)});
    },
    [=](wait_atom) {
      // nop 
    },
    [=](start_atom) {
      // nop 
    },
    [=](done_atom) {
      self->send(factory_actor, done_atom::value);
      self->quit();
    },
  };
}

behavior costumer_factory_actor(event_based_actor* self,
                                atomic_long* id_generator, int haircuts,
                                actor room) {
  default_random_engine random{};
  int num_hair_cuts_so_far = 0;
  auto send_customer_to_room = [=](actor&& customer) {
    auto enter_message = enter{move(customer), room};
    self->send(room, move(enter_message));
  };
  auto send_to_room = [=]() {
    auto customer = self->spawn(customer_actor, id_generator->fetch_add(1),
                                actor_cast<actor>(self));
    send_customer_to_room(move(customer));
  };
  return  {
    [=](start_atom) mutable {
      for (int i = 0; i < haircuts; ++i) {
        send_to_room();
        busy_wait(next_int(random, config::apr) + 10);
      } 
    },
    [=](returned& msg) {
      id_generator->fetch_add(1); 
      send_customer_to_room(move(msg.customer));
    }, 
    [=](done_atom) mutable {
      ++num_hair_cuts_so_far; 
      if (num_hair_cuts_so_far == haircuts) {
        cout << "Total attempts: " << *id_generator << endl;
        self->send(room, exit_atom::value);
        self->quit();
      }
    } 
  };
}

void caf_main(actor_system& system, const config& cfg) {
  atomic_long id_generator(0);
  auto barber = system.spawn(barber_actor);
  auto room = system.spawn(waiting_room_actor, cfg.w, barber);
  auto factory_actor =
    system.spawn(costumer_factory_actor, &id_generator, cfg.n, room);
  anon_send(factory_actor, start_atom::value);
}

CAF_MAIN()
