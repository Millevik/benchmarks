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

#include "savina_helper.hpp"

using namespace std;
using std::chrono::seconds;
using namespace caf;

#define CAF_ALLOW_UNSAFE_MESSAGE_TYPE(type_name)                               \
  namespace caf {                                                              \
  template <>                                                                  \
  struct allowed_unsafe_message_type<type_name> : std::true_type {};           \
  }

class config : public actor_system_config {
public:
  static array<double, 10> ref_val;
  static array<int, 10> data_sizes;
  static int jacobi_num_iter;
  static double omega;
  static long random_seed;
  static int n;

  static matrix2d<double> a;
  static random_gen r;
  config() {
    opt_group{custom_options_, "global"}
      .add(n, "nnn,n", "input size");
  }

  void initialize() const {
    r = reset_random_generator();
    int data_size = data_sizes[n];
    a = random_matrix(data_size, data_size);
  }

  static random_gen reset_random_generator() {
    return random_gen(random_seed);
  }

  static void perform_computation(double theta) {
    double sint = sin(theta);
    double res = sint * sint;
    //defeat dead code elimination
    if (res <= 0) {
      cerr << "Benchmark exited with unrealistic res value " << res << endl;
      exit(1);
    }
  }

  static matrix2d<double> random_matrix(int m, int n) {
    matrix2d<double> a(m, n);
    for (int i = 0; i < m; i++) {
      for (int j = 0; j < n; j++) {
        a(i, j) = r.next_double() * 1e-6;
      }
    }
    return a;
  }

  static void jgf_validate(double g_total, int size) {
    double dev = abs(g_total - ref_val[size]);
    if (dev > 1.0e-12) {
      cout << "Validation failed" << endl;
      cout << "gtotal = " << g_total << "  " << ref_val[size] << "  " << dev
           << "  " << size << endl;
    } else {
      cout << "Validation OK!" << endl;
    }
  }
};
array<double, 10> config::ref_val = {
    {0.000003189420084871275,
    0.001846644602759566,
    0.0032099996270638005,
    0.0050869220175413146,
    0.008496328291240363,
    0.016479973604143234,
    0.026575660248076397,
    1.026575660248076397,
    2.026575660248076397,
    3.026575660248076397}
};
array<int, 10> config::data_sizes = {
  {20, 80, 100, 120, 150, 200, 250, 300, 350, 400}};
int config::jacobi_num_iter = 100;
double config::omega = 1.25;
long config::random_seed = 10101010;
int config::n = 0;
matrix2d<double> config::a;
random_gen config::r;

struct sor_border {
  vector<actor> border_actors;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(sor_border);

struct sor_border_msg{ 
  sor_border m_border;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(sor_border_msg);

struct sor_start_msg { 
  int mi;
  vector<actor> m_actors;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(sor_start_msg);

struct sor_value_msg { 
  double v;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(sor_value_msg);

using sor_boot_msg_atom = atom_constant<atom("sorboot")>;

struct sor_result_msg { 
  int mx; 
  int my; 
  double mv;
  int msg_rcv;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(sor_result_msg);

struct sor_actor_state {
  double value; 
  int x;
  int y;
  double omega_over_four;
  double one_minus_omega;
  vector<int> neighbors;
  int iter;
  int max_iter;
  int msg_rcv;
  vector<actor> sor_actors;
  int received_vals;
  double sum;
  bool expecting_start;
  list<sor_value_msg> pending_messages;
};

behavior sor_actor_fun(stateful_actor<sor_actor_state>* self, int pos,
                       double value_, int color, int nx, int ny, double omega,
                       actor sor_source, bool /*peer*/) {
  auto& s = self->state;
  s.value = value_; //variable value is used for storing data;
  s.x = pos / ny;
  s.y = pos % ny;
  s.omega_over_four = 0.25 * omega;
  s.one_minus_omega = 1.0 - omega;
  auto cal_pos = [=](int x1, int y1) {
    return x1 * ny + y1;
  };
  auto neighbors_tmp_fun = [=]() {
    auto& s = self->state;
    if (s.x > 0 && s.x < nx - 1 && s.y > 0 && s.y < ny - 1) {
      vector<int> temp_neighbors(4);
      temp_neighbors[0] = cal_pos(s.x, s.y + 1);
      temp_neighbors[1] = cal_pos(s.x + 1, s.y);
      temp_neighbors[2] = cal_pos(s.x, s.y - 1);
      temp_neighbors[3] = cal_pos(s.x - 1, s.y);
      return temp_neighbors;
    } else if ((s.x == 0 || s.x == (nx - 1)) && (s.y == 0 || s.y == (ny - 1))) {
      vector<int> temp_neighbors(2);
      temp_neighbors[0] = (s.x == 0) ? cal_pos(s.x + 1, s.y) : cal_pos(s.x - 1, s.y);
      temp_neighbors[1] = (s.y == 0) ?cal_pos(s.x, s.y + 1) : cal_pos(s.x, s.y - 1);
      return temp_neighbors;
    } else if ((s.x == 0 || s.x == (nx - 1)) || (s.y == 0 || s.y == (ny - 1))) {
      vector<int> temp_neighbors(3);
      if (s.x == 0 || s.x == nx - 1) {
        temp_neighbors[0] = (s.x == 0) ? cal_pos(s.x + 1, s.y) : cal_pos(s.x - 1, s.y);
        temp_neighbors[1] = cal_pos(s.x, s.y + 1);
        temp_neighbors[2] = cal_pos(s.x, s.y - 1);
      } else {
        temp_neighbors[0] = (s.y == 0) ? cal_pos(s.x, s.y + 1) : cal_pos(s.x, s.y - 1);
        temp_neighbors[1] = cal_pos(s.x + 1, s.y);
        temp_neighbors[2] = cal_pos(s.x - 1, s.y);
      }
      return temp_neighbors;
    } else {
      return vector<int>();
    }
  };
  s.neighbors = neighbors_tmp_fun();
  s.iter = 0;
  s.max_iter = 0;
  s.msg_rcv = 0;
  s.received_vals = 0;
  s.sum = 0.0;
  s.expecting_start = true;
  return {
    [=](sor_start_msg& msg) {
      auto& s = self->state;
      s.expecting_start = false;
      s.sor_actors = msg.m_actors;
      s.max_iter = msg.mi;
      if (color == 1) {
        for (auto loop_neigh_index : s.neighbors) {
          self->send(s.sor_actors[loop_neigh_index], sor_value_msg{s.value});
        }
        ++s.iter;
        ++s.msg_rcv;
      }
      for (auto& loop_msg : s.pending_messages) {
        self->send(actor_cast<actor>(self), loop_msg);
      }
      s.pending_messages.clear();
    },
    [=](sor_value_msg& message) {
      auto& s = self->state;
      if (s.expecting_start) {
        s.pending_messages.emplace_back(message);
      } else {
        ++s.msg_rcv;
        if (s.iter < s.max_iter) {
          ++s.received_vals;
          s.sum += message.v;
          if (static_cast<size_t>(s.received_vals) == s.neighbors.size()) {
            s.value =
              (s.omega_over_four * s.sum) + (s.one_minus_omega * s.value);
            s.sum = 0.0;
            s.received_vals = 0;
            for(auto loop_neigh_index : s.neighbors) {
              self->send(s.sor_actors[loop_neigh_index],
                         sor_value_msg{s.value});
            }
            ++s.iter;
          }
          if (s.iter == s.max_iter) {
            self->send(sor_source,
                       sor_result_msg{s.x, s.y, s.value, s.msg_rcv});
            self->quit();
          }
        }
      }
    }
  };
}

struct sor_peer_state {
  vector<actor> sor_actors;
  double g_total;
  int returned;
  int total_msg_rcv;
  bool expecting_boot;
};

behavior sor_peer_fun(stateful_actor<sor_peer_state>* self, int s,
                      int part_start, matrix2d<double> matrix_part,
                      sor_border border, actor sor_source) {
  auto& t = self->state;
  t.sor_actors = vector<actor>(s * (s - part_start + 1));
  auto boot = [=]() {
    auto& t = self->state;
    vector<actor> my_border(s);
    for (int i = 0; i < s; ++i) {
      t.sor_actors[i * (s - part_start + 1)] = border.border_actors[i];
    }
    for (int i = 0; i < s; ++i) {
      auto c = (i + part_start) % 2;
      for (int j = 1; j < (s - part_start + 1); ++j) {
        auto pos = i * (s - part_start + 1) + j;
        c = 1 - c;
        t.sor_actors[pos] = self->spawn(
          sor_actor_fun, pos, matrix_part(i, j - 1), c, s, s - part_start + 1,
          config::omega, actor_cast<actor>(self), true);
        if (j == 1) {
          my_border[i] = t.sor_actors[pos];
        }
      }
    }
    self->send(sor_source, sor_border_msg{sor_border{move(my_border)}});
    for (int i = 0; i < s; ++i) {
      for (int j = 1; j < (s - part_start + 1); ++j) {
        auto pos = i * (s - part_start + 1) + j;
        self->send(t.sor_actors[pos],
                   sor_start_msg{config::jacobi_num_iter, t.sor_actors});
      }
    }
  };
  t.g_total = 0.0;
  t.returned = 0;
  t.total_msg_rcv = 0;
  t.expecting_boot = true;
  return {
    [=](sor_boot_msg_atom) {
      self->state.expecting_boot = false;
      boot();
    },
    [=](sor_result_msg& msg) { //(mx, my, mv, msg_rcv) 
      auto& t = self->state;
      if (t.expecting_boot) {
        cerr << "SorPeer not booted yet!" << endl;
        exit(1);
      }
      t.total_msg_rcv += msg.msg_rcv;
      ++t.returned;
      t.g_total += msg.mv;
      if (t.returned == s * (s - part_start)) {
        self->send(sor_source,
                   sor_result_msg{-1, -1, t.g_total, t.total_msg_rcv});
        self->quit();
      }
    }
  };
}

struct sor_runner_state {
  int s;
  int part;
  vector<actor> sor_actors;
  double g_total;
  int returned;
  int total_msg_rcv;
  bool expecting_boot;
};

behavior sor_runner_fun(stateful_actor<sor_runner_state>* self, int n) {
  auto& t = self->state;
  t.s = config::data_sizes[n];
  t.part = t.s / 2;
  t.sor_actors = vector<actor>(t.s * (t.part + 1));
  auto boot = [=]() {
    auto& t = self->state;
    vector<actor> my_border(t.s);
    const auto& randoms = config::a;
    for (int i = 0; i < t.s; ++i) {
      auto c = i % 2;
      for (int j = 0; j < t.part; ++j) {
        auto pos = i * (t.part + 1) + j;
        c = 1 - c;
        t.sor_actors[pos] =
          self->spawn(sor_actor_fun, pos, randoms(i, j), c, t.s, t.part + 1,
                      config::omega, actor_cast<actor>(self), false);
        if (j == (t.part - 1)) {
          my_border[i] = t.sor_actors[pos];
        }
      }
    }
    matrix2d<double> partial_matrix(t.s, t.s - t.part);
    for (int i = 0; i < t.s; ++i) {
      for (int j = 0; j < t.s - t.part; ++j) {
        partial_matrix(i, j) = randoms(i, j + t.part);
      }
    }
    auto sor_peer =
      self->spawn(sor_peer_fun, t.s, t.part, partial_matrix,
                  sor_border{move(my_border)}, actor_cast<actor>(self));
    self->send(sor_peer, sor_boot_msg_atom::value);
  };
  t.g_total = 0.0;
  t.returned = 0;
  t.total_msg_rcv = 0;
  t.expecting_boot = true;
  return {
    [=](sor_boot_msg_atom) {
      self->state.expecting_boot = false;
      boot();
    },
    [=](sor_result_msg& msg) {
      auto& t = self->state;
      if (t.expecting_boot) {
        cerr << "SorRunner not booted yet!" << endl;
        exit(1);
      }
      t.total_msg_rcv += msg.msg_rcv;
      t.returned += 1;
      t.g_total += msg.mv;
      if (t.returned == (t.s * t.part) + 1) {
        config::jgf_validate(t.g_total, n);
        self->quit();
      }
    },
    [=](sor_border_msg& msg) {
      auto& t = self->state;
      if (t.expecting_boot) {
        cerr << "SorRunner not booted yet!" << endl;
        exit(1);
      }
      for (int i = 0; i < t.s; ++i) {
        t.sor_actors[(i + 1) * (t.part + 1) - 1] =
          move(msg.m_border.border_actors[i]);
      }
      for (int i = 0; i < t.s; ++i) {
        for (int j = 0; j < t.part; ++j) {
          auto pos = i * (t.part + 1) + j;
          self->send(t.sor_actors[pos],
                     sor_start_msg{config::jacobi_num_iter, t.sor_actors});
        }
      }
    }
  };
}

void caf_main(actor_system& system, const config& cfg) {
  cerr << "savina implementation not working..." << endl;
  exit(1);
  cfg.initialize();
  auto data_level = cfg.n;
  auto sor_runner = system.spawn(sor_runner_fun, data_level);
  anon_send(sor_runner, sor_boot_msg_atom::value);
  cout << "pre-AkkaActorState.awaitTermination" << endl;
  system.await_all_actors_done();
  cout << "post-AkkaActorState.awaitTermination" << endl;
}

CAF_MAIN()
