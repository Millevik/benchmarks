
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

using procs_t = vector<actor>;

class pinger : public event_based_actor {
public:
  pinger(actor_config& cfg) : event_based_actor(cfg) {
    this->set_default_handler(skip);
  }

  behavior_type make_behavior() override {
    return b_E_E_true_;
  }
private:
  std::function<void(ping_atom)> f_answer_ping_ = {
    [this] (ping_atom) {
      this->send(actor_cast<actor>(this->current_sender()), pong_atom::value);
    } 
  };

  std::function<void(pong_atom)> f_react_to_pong_ = {
    [this] (pong_atom) {
      auto it =
        find(begin(pinged_procs_), end(pinged_procs_), current_sender());
      if (it != pinged_procs_.end()) {
        pinged_procs_.erase(it); 
        if (pinged_procs_.empty()) { //b_E_E_ReportTo
          this->send(report_to_, done_atom::value);
          this->become(b_E_E_false);
        }
      }
    } 
  };

  message_handler b_E_E_true_ = {
    [=] (procs_atom, procs_t& procs, actor report_to) {
      procs_ = procs;
      pinged_procs_.reserve(procs_.size());
      report_to_ = report_to;
      ping_it_ = procs_.begin();
      this->become(b_F_F_ReportTo_);
    }
  };

  message_handler b_E_E_false = {
    f_answer_ping_,
    [=] (die_atom) {
      //this->quit();
    }
  };

  message_handler b_E_F_ReportTo = {
    f_answer_ping_,
    f_react_to_pong_ 
  };

  behavior b_F_F_ReportTo_ = {
    f_answer_ping_,
    after(std::chrono::seconds(0)) >> [=] {
      this->send(*ping_it_, ping_atom::value);
      pinged_procs_.emplace_back(*ping_it_);
      ++ping_it_;
      if (ping_it_ == procs_.end()) {
        procs_.clear();
        this->become(b_E_F_ReportTo); 
        return;
      }
    }
  };

  procs_t procs_;
  procs_t pinged_procs_;
  actor report_to_;
  procs_t::iterator ping_it_;
};


void receive_msgs(scoped_actor& self, procs_t procs) {
  while (!procs.empty()) {
    self->receive(
      [&] (done_atom) {
        auto it = find(begin(procs), end(procs),
                       actor_cast<actor>(self->current_sender()));
        if (it != procs.end()) {
          procs.erase(it); 
        }
     });
  }
}

void send_procs(scoped_actor& self, procs_t& procs) {
  for (auto& p: procs) {
    self->send(p, procs_atom::value, procs, self);
  }
}

procs_t spawn_procs(actor_system& system, int n) {
  procs_t result;
  result.reserve(n);
  for (int i = 0; i < n; ++i) {
    //result.emplace_back(system.spawn<pinger>()) ;
    result.emplace_back(system.spawn<pinger, lazy_init>()) ;
  }
  return result;
}

void run(actor_system& system, int n) {
  auto procs = spawn_procs(system, n);
  scoped_actor self{system};
  send_procs(self, procs);
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
    std::cerr << "version musst be short,intermediate or long" << std::endl;
    exit(1);
  }
  int cores = std::stoi(argv[2]);
  actor_system_config cfg;
  cfg.parse(argc, argv, "caf-application.ini");
  actor_system system{cfg};
  int n = f * cores;
  run(system, n); 
}

