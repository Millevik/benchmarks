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
#include <stdlib.h>

#include "caf/all.hpp"

using namespace std;
using namespace caf;

#define CAF_ALLOW_UNSAFE_MESSAGE_TYPE(type_name)                               \
  namespace caf {                                                              \
  template <>                                                                  \
  struct allowed_unsafe_message_type<type_name> : std::true_type {};           \
  }

enum color_t {
  red, blue, yellow, faded
};

const char* color_str[] = {
  "red", "blue", "yellow", "faded"
};

string to_string(color_t c) {
  return color_str[c];
}

struct chameneos_helper {
  static color_t complement(color_t color, color_t other_color) {
    switch (color) {
      case red:
        switch (other_color) {
          case red:
            return red;
          case yellow:
            return blue;
          case blue:
            return yellow;
          case faded:
            return faded;
        }
        break;
      case yellow:
        switch (other_color) {
          case red:
            return blue;
          case yellow:
            return yellow;
          case blue:
            return red;
          case faded:
            return faded;
        }
        break;
      case blue:
        switch (other_color) {
          case red:
            return yellow;
          case yellow:
            return red;
          case blue:
            return blue;
          case faded:
            return faded;
        }
        break;
      case faded:
        return faded;
    }
    throw string("Unknown color: " + to_string(color));
  }

  static color_t faded_color() {
    return faded;
  }

  struct meet_msg {
    color_t color;
    //actor sender;
  };

  struct change_msg {
    color_t color;
    //actor sender;
  };

  struct meeting_count_msg {
    int count;
    //actor sender;
  };

  struct exit_msg {
    //actor sender;
  };
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(chameneos_helper::meet_msg);
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(chameneos_helper::change_msg);
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(chameneos_helper::meeting_count_msg);
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(chameneos_helper::exit_msg);

class config : public actor_system_config {
public:
  int num_chameneos = 100;
  int num_meetings = 200000;

  config() {
    opt_group{custom_options_, "global"}
    .add(num_chameneos, "ccc,c", "number of chameneos")
    .add(num_meetings, "mmm,m", "number of meetings");
  }
};

struct chameneos_chameneo_actor_state {
  int meetings;
  color_t color;
};

behavior
chameneos_chameneo_actor(stateful_actor<chameneos_chameneo_actor_state>* self,
                         actor mall, color_t color_, int /*id*/) {
  auto& s = self->state;
  s.meetings = 0;
  s.color = color_;
  self->send(mall, chameneos_helper::meet_msg{s.color});
  return  {
    [=](chameneos_helper::meet_msg& msg) {
      auto& s = self->state;
      auto other_color = msg.color; 
      auto sender = actor_cast<actor>(self->current_sender());
      s.color = chameneos_helper::complement(s.color, other_color);
      ++s.meetings;
      self->send(sender, chameneos_helper::change_msg{s.color});
      self->send(mall, chameneos_helper::meet_msg{s.color});
    },
    [=](chameneos_helper::change_msg& msg) {
      auto& s = self->state;
      s.color = msg.color;
      ++s.meetings;
      self->send(mall, chameneos_helper::meet_msg{s.color});
    },
    [=](chameneos_helper::exit_msg& /*msg*/) {
      auto& s = self->state;
      auto sender = actor_cast<actor>(self->current_sender());
      s.color = chameneos_helper::faded_color();
      self->send(sender,
                 chameneos_helper::meeting_count_msg{s.meetings});
      self->quit();
    }
  };
}

struct chameneos_mall_actor_state {
  actor waiting_chameneo;
  int sum_meetings;
  int num_faded;
  int n;
};

behavior chameneos_mall_actor(stateful_actor<chameneos_mall_actor_state>* self,
                              int n_, int num_chameneos) {
  auto& s = self->state;
  s.n = n_;
  auto actor_self = actor_cast<actor>(self);
  //start_chameneos
  for (int i = 0; i < num_chameneos; ++i) {
    auto color = static_cast<color_t>(i % 3);
    auto loop_chamenos = self->spawn(chameneos_chameneo_actor, actor_self, color, i);
  }
  return {
    [=](chameneos_helper::meeting_count_msg& msg) {
      auto& s = self->state; 
      ++s.num_faded;
      s.sum_meetings += msg.count;
      if (s.num_faded == num_chameneos) {
        self->quit(); 
      }
    },
    [=](chameneos_helper::meet_msg& msg) {
      auto& s = self->state; 
      if (s.n > 0) {
        if (!s.waiting_chameneo) {
          s.waiting_chameneo = actor_cast<actor>(self->current_sender());
        } else {
          --s.n;
          self->send(s.waiting_chameneo, msg);
          destroy(s.waiting_chameneo);
        } 
      } else {
        self->send(actor_cast<actor>(self->current_sender()), chameneos_helper::exit_msg{});
      }
    } 
  };
}


void caf_main(actor_system& system, const config& cfg) {
  auto mall_actor =
    system.spawn(chameneos_mall_actor, cfg.num_meetings, cfg.num_chameneos);
}

CAF_MAIN()
