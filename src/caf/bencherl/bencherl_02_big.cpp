
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
#include <thread> //sleep_for
#include <chrono>

#include "caf/all.hpp"

using namespace std;
using namespace caf;


using done_atom = atom_constant<atom("done")>;
using procs_atom = atom_constant<atom("procs")>;
using die_atom = atom_constant<atom("die")>;
using ping_atom = atom_constant<atom("ping")>;
using pong_atom = atom_constant<atom("pong")>;

struct pinger_state {
  set<actor> procs;
  actor report_to;
};

behavior pinger(stateful_actor<pinger_state>* self) {
  return {
    [=] (procs_atom, set<actor>& procs, actor report_to) {
      self->state.procs.swap(procs);
      self->state.report_to = report_to;
      for (auto& p : self->state.procs) {
        self->request(p, infinite, ping_atom::value).then(
          [=](pong_atom) {
            auto it =
              self->state.procs.find(p);
            if (it != self->state.procs.end()) {
              self->state.procs.erase(it);
            } else {
              cerr << "Error should never be reached" << endl;
              // to indicate this error we wait for a long time
              // similar to bencherl big
              this_thread::sleep_for(std::chrono::hours::max());
            }
            if (self->state.procs.empty()) {
              self->send(self->state.report_to, done_atom::value);
            }
          }
        );
      }
    },
    [=] (ping_atom) {
      return pong_atom::value; 
    },
    [=] (die_atom) {
      //self->quit(); 
    }
  };
}

void receive_msgs(scoped_actor& self, set<actor> procs) {
  while(!procs.empty()){
    self->receive(
      [&] (done_atom) {
        auto it = procs.find(actor_cast<actor>(self->current_sender()));
        if (it != procs.end()) {
          procs.erase(it); 
        }
      }
    );
  }
}

void send_procs(set<actor>& procs, scoped_actor& self) {
  for (auto& p: procs) {
    self->send(p, procs_atom::value, procs, self);
  }
}

set<actor> spawn_procs(actor_system& system, int n) {
  set<actor> result;
  for (int i = 0; i < n; ++i) {
    result.insert(system.spawn(pinger)) ;
  }
  return result;
}

void run(actor_system& system, int n) {
  auto procs = spawn_procs(system, n);
  scoped_actor self{system};
  send_procs(procs, self);
  receive_msgs(self, procs);
  for (auto& p: procs) {
    self->send(p, die_atom::value);
  }
}

void usage() {
  cout << "usage: bencherl_02_big VERSION NUM_CORES" << endl
       << "       VERSION:      short|intermediate|long " << endl
       << "       NUM_CORES:    number of cores" << endl << endl
       << "  for details see http://release.softlab.ntua.gr/bencherl/" << endl;
  exit(1);
}

int main(int argc, char** argv) {
  // configuration
  if (argc != 3)
    usage();
  string version = argv[1];
  int f;
  if (version == "test") {
    f = 1;
  } else if (version == "short") {
    f = 8;
  } else if (version == "intermediate") {
    f = 16;
  } else if (version == "long") {
    f = 24;
  } else {
    std::cerr << "version musst be short,intermediate or long" << std::endl; ;
    exit(1);
  }
  int cores = std::stoi(argv[2]);
  actor_system_config cfg;
  cfg.parse(argc, argv, "caf-application.ini");
  actor_system system{cfg};
  int n = f * cores;
  run(system, n); 
}

