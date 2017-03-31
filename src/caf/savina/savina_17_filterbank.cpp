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

enum class message_channel {
  C00,
  C01,
  C02,
  C03,
  C04,
  C05,
  C06,
  C07,
  C08,
  C09,
  C10,
  C11,
  C12,
  C13,
  C14,
  C15,
  C16,
  C17,
  C18,
  C19,
  C20,
  C21,
  C22,
  C23,
  C24,
  C25,
  C26,
  C27,
  C28,
  C29,
  C30,
  C31,
  C32,
  C33,
  C_LOWEST,
  COUNT // reptresents to values().length in java 
};

template<class T>
struct matrix_t {
  void init(int y, int x) {
    m.reserve(y);
    for (int i = 0;  i < y; ++i) {
      vector<T> tmp(x);
      m.emplace_back(move(tmp));
    }
  }
  vector<vector<T>> m; // m[y][x]
};


class config : public actor_system_config {
public:

  int num_columns = 8192 *2;
  int num_simulations = 2048 + (max(2048, num_columns) * 2);
  static int num_channels; // = 8;
  int sink_print_rate = 100;
  static matrix_t<double> h;
  static matrix_t<double> f;
  static bool debug; // = false;

  config() {
    opt_group{custom_options_, "global"}
      .add(num_simulations, "sss,s", "number of simulations")
      .add(num_columns, "ccc,c", "number of columns")
      .add(num_channels, "aaa,a", "number of channels")
      .add(debug, "ddd,d", "debug");
  }

  void initalize() const {
    int arg_int = config::num_channels;
    int max_channels = static_cast<int>(message_channel::COUNT) -1;
    num_channels = max(2, min(arg_int, max_channels));
    h.init(num_channels, num_columns);
    f.init(num_channels, num_columns);
    for (int j = 0; j < num_channels; j++) {
      for (int i = 0; i < num_columns; i++) {
        h.m[j][i] =
          (1.0 * i * num_columns) + (1.0 * j * num_channels) + j + i + j + 1;
        f.m[j][i] = (1.0 * i * j) + (1.0 * j * j) + j + i;
      }
    }
  }

};
int config::num_channels= 8;
matrix_t<double> config::h;
matrix_t<double> config::f;
bool config::debug = false;

struct next_msg {
  actor source;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(next_msg);

using boot_msg_atom = atom_constant<atom("bootmsg")>;
using exit_msg_atom = atom_constant<atom("exitmsg")>;

struct value_msg {
  double value;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(value_msg);

struct source_value_msg {
  int source_id;
  double value;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(source_value_msg);

struct collection_msg {
  vector<double> values; 
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(collection_msg);

behavior producer_actor_fun(event_based_actor* self, int num_simulations) {
  auto num_messages_sent = 0;
  return {
    [=](next_msg& message) mutable {
      auto& source = message.source; 
      if (num_messages_sent == num_simulations) {
        self->send(source, exit_msg_atom::value); 
        self->quit(); 
      } else {
        self->send(source, boot_msg_atom::value); 
        ++num_messages_sent;
      }
    }
  };
}

behavior source_actor_fun(event_based_actor* self, actor producer,
                          actor next_actor) {
  const auto max_value = 1000;
  auto current = 0;
  return {
    [=](boot_msg_atom) mutable {
      self->send(next_actor, value_msg{static_cast<double>(current)});
      current = (current + 1) % max_value;
      self->send(producer, next_msg{static_cast<actor>(self)});
    },
    [=](exit_msg_atom) {
      // onPostExit()
      self->send(next_actor, exit_msg_atom::value);
      self->quit(); 
    }
  };
}

behavior sink_actor_fun(event_based_actor* self, int print_rate) {
  auto count = 0;
  return {
    [=](value_msg& message) mutable  {
      auto result = message.value; 
      if (config::debug && count == 0) {
        cout << "SinkActor: result = " << result << endl;
      }
      count = (count + 1) % print_rate;
    },
    [=](exit_msg_atom) {
      self->quit(); 
    }
  };
}

struct delay_actor_state {
  vector<double> state;
  int place_holder;
};

behavior delay_actor_fun(stateful_actor<delay_actor_state>* self,
                         string /*source_id*/, int delay_length,
                         actor next_actor) {
  auto& s = self->state;
  s.state.reserve(delay_length);
  for (int i = 0; i < delay_length; ++i) {
    s.state.emplace_back(0); 
  }
  s.place_holder = 0;
  return {
    [=](value_msg& message) {
      auto& s = self->state;
      auto result = message.value; 
      self->send(next_actor, value_msg{s.state[s.place_holder]});
      s.state[s.place_holder] = result;
      s.place_holder = (s.place_holder + 1) % delay_length;
    },
    [=](exit_msg_atom) {
      // onPostExit()
      self->send(next_actor, exit_msg_atom::value);
      self->quit(); 
    },
  };
}

struct fir_filter_actor_state {
  vector<double> data;
  vector<double> coefficients;
  int data_index;
  bool data_full;
};

behavior fir_filter_actor_fun(stateful_actor<fir_filter_actor_state>* self,
                              string /*source_id*/, int peek_length,
                              vector<double> coefficients_, actor next_actor) {
  auto& s = self->state;
  s.data.reserve(peek_length);
  for (int i = 0; i < peek_length; ++i) {
    s.data.emplace_back(0); 
  }
  s.data_index = 0;
  s.data_full = false;
  s.coefficients = move(coefficients_);
  return {
    [=](value_msg& message) {
      auto& s = self->state;
      auto result = message.value;
      s.data[s.data_index] = result;
      ++s.data_index;
      if (s.data_index == peek_length) {
        s.data_full = true;
        s.data_index = 0;
      }
      if (s.data_full) {
        auto sum = 0.0; 
        for (int i = 0; i < peek_length; ++i) {
          sum += s.data[i] * s.coefficients[peek_length - i - 1] ;
        }
        self->send(next_actor, value_msg{sum});
      }
    }, 
    [=](exit_msg_atom) {
      self->send(next_actor, exit_msg_atom::value);
      self->quit();
    } 
  };
}

behavior sample_filter_actor_fun(event_based_actor* self, int sample_rate,
                                 actor next_actor) {
  auto samples_received = 0;
  return {
    [=](value_msg& the_msg) mutable {
      if (samples_received == 0) {
        self->send(next_actor, move(the_msg)); 
      } else {
        self->send(next_actor, value_msg{0}); // ZERO_RESULT
      }
      samples_received = (samples_received + 1) % sample_rate;
    },
    [=](exit_msg_atom) {
      // onPostexit()
      self->send(next_actor, exit_msg_atom::value);
      self->quit(); 
    }
  };
}

behavior tagged_forward_actor_fun(event_based_actor* self, int source_id,
                                  actor next_actor) {
  return {
    [=](value_msg& message) {
      auto result = message.value; 
      self->send(next_actor, source_value_msg{source_id, result});
    },
    [=](exit_msg_atom) {
      // onPostExit()
      self->send(next_actor, exit_msg_atom::value);
      self->quit(); 
    }
  };
}

struct integrator_actor_state {
  vector<map<int, double>> data;
  int exits_received = 0;
};

behavior integrator_actor_fun(stateful_actor<integrator_actor_state>* self,
                              int num_channels, actor next_actor) {
  return {
    [=](source_value_msg& message) {
      auto& s = self->state;
      auto source_id = message.source_id;
      auto result = message.value;
      auto data_size = s.data.size();
      auto processed = false;
      for (size_t i = 0; i < data_size; ++i) {
        auto& loop_map = s.data[i];
        auto tmp_it = loop_map.find(source_id);
        if (tmp_it == end(loop_map)) {
          loop_map[source_id] = result;
          processed = true;
          i = data_size;
        }
      }
      if (!processed) {
        map<int, double> new_map; 
        new_map[source_id] = result;
        s.data.emplace_back(move(new_map));
      }
      auto& first_map = *begin(s.data);
      if (static_cast<int>(first_map.size()) == num_channels) {
        vector<double> values;
        values.reserve(first_map.size());
        for (auto& kv : first_map) {
          values.emplace_back(kv.second); 
        }
        self->send(next_actor, collection_msg{move(values)});
        s.data.erase(begin(s.data));
      }
    },
    [=](exit_msg_atom) {
      // onPostExit()
      auto& s = self->state;
      ++s.exits_received;
      if (s.exits_received == num_channels) {
        self->send(next_actor, exit_msg_atom::value);
        self->quit(); 
      }
    }
  };
}

behavior combine_actor_fun(event_based_actor* self, actor next_actor) {
  return {
    [=](collection_msg& message) {
      auto& result = message.values;
      double sum = 0;
      for (auto loop_value : result) {
        sum += loop_value; 
      }
      self->send(next_actor, value_msg{sum});
    },
    [=](exit_msg_atom) {
      // onPostExit() 
      self->send(next_actor, exit_msg_atom::value);
      self->quit();
    }
  };
}

behavior bank_actor_fun(event_based_actor* self, int source_id, int num_columns,
                        vector<double> h, vector<double> f, actor integrator) {
  auto first_actor = self->spawn(delay_actor_fun, to_string(source_id) + ".1", num_columns -1,
    self->spawn(fir_filter_actor_fun, to_string(source_id) + ".1", num_columns, move(h),
      self->spawn(sample_filter_actor_fun, num_columns, 
        self->spawn(delay_actor_fun, to_string(source_id) + ".2", num_columns -1,
          self->spawn(fir_filter_actor_fun, to_string(source_id) + ".2", num_columns, move(f),
            self->spawn(tagged_forward_actor_fun, source_id, integrator))))));
  return {
    [=](value_msg& the_msg) {
      self->send(first_actor, move(the_msg));
    },
    [=](exit_msg_atom) {
      // onPostExit()
      self->send(first_actor, exit_msg_atom::value);
      self->quit();
    }
  };
}


struct branches_actor_state {
  vector<actor> banks;
};

behavior branches_actor_fun(stateful_actor<branches_actor_state>* self,
                            int num_channels, int num_columns,
                            matrix_t<double> h, matrix_t<double> f,
                            actor next_actor) {
  auto& s = self->state; 
  s.banks.reserve(num_channels);
  for (int i = 0; i < num_channels; ++i) {
    s.banks.emplace_back(
      self->spawn(bank_actor_fun, i, num_columns, h.m[i], f.m[i], next_actor));
  }
  return {
    [=](const value_msg& the_msg) {
      for (auto& loop_bank : self->state.banks) {
        self->send(loop_bank, the_msg);
      } 
    },
    [=](exit_msg_atom) {
      // onPostExit()
      for (auto& loop_bank : self->state.banks) {
        self->send(loop_bank, exit_msg_atom::value); 
      }
      self->quit(); 
    }
  };
}

void caf_main(actor_system& system, const config& cfg) {
  cfg.initalize();
  auto num_simulations = cfg.num_simulations;
  auto num_channels = cfg.num_channels;
  auto num_columns = cfg.num_columns;
  auto& h = cfg.h;
  auto& f = cfg.f;
  auto sink_print_rate = cfg.sink_print_rate;
  // create the pipeline of actors
  auto producer = system.spawn(producer_actor_fun, num_simulations); 
  auto sink = system.spawn(sink_actor_fun, sink_print_rate);
  auto combine = system.spawn(combine_actor_fun, sink);
  auto integrator = system.spawn(integrator_actor_fun, num_channels, combine);
  auto branches = system.spawn(branches_actor_fun, num_channels, num_columns, h, f, integrator);
  auto source = system.spawn(source_actor_fun, producer, branches);
  // start the pipeline
  anon_send(producer, next_msg{source});
  system.await_all_actors_done();
}

CAF_MAIN()
