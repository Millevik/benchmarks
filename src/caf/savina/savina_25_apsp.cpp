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



class config : public actor_system_config {
public:
  static int n; // = 300
  static int b; // = 50
  static int w; // = 100;

  config() {
    opt_group{custom_options_, "global"}
      .add(n, "nnn,n", "number of workers")
      .add(b, "bbb,b", "block size")
      .add(w, "www,w", "maximum edge weight");
  }
};
int config::n = n;
int config::b = b;
int config::w = w;

int next_int(default_random_engine& r,
             int exclusive_max = std::numeric_limits<int>::max()) {
  return r() % exclusive_max;
}

using arr2l = vector<vector<long>>;
arr2l array_tabulate(size_t y_size, size_t x_size, long init_value) {
  arr2l result;
  result.reserve(y_size);
  for (size_t y = 0; y < y_size; ++y) {
    result.emplace_back(x_size, init_value);
  }
  return result;
}

struct apsp_utils {
  arr2l graph_data;

  void generate_graph() {
    auto n = config::n; 
    auto w = config::w;
     
    default_random_engine random(n);
    arr2l local_data = array_tabulate(n, n, 0);
    for (int i = 0; i < n; ++i) {
      for (int j = i + 1; j < n; ++j) {
        auto r = next_int(random, w) + 1; 
        local_data[i][j] = r;
        local_data[j][i] = r;
      }
    }
    graph_data = move(local_data);
  }

  arr2l get_block(const arr2l& src_data, int my_block_id) {
    auto n = config::n; 
    auto b = config::b;
    auto local_data = array_tabulate(b, b, 0);
    auto num_blocks_per_dim = n / b;
    auto global_start_row = (my_block_id / num_blocks_per_dim) * b;
    auto global_start_col = (my_block_id % num_blocks_per_dim) * b;
    for (int i = 0; i < b; ++i) {
      for (int j = 0; j < b; ++j) {
        local_data[i][j] = src_data[i + global_start_row][j + global_start_col];
      }
    }
    return local_data;
  }

  void print(const arr2l& array) {
    ostringstream ss;
    for (auto& a : array) {
      for (auto b : a) {
        ss << b << " ";
      } 
      ss << endl;
    }
    cout << ss.str() << endl;
  }

  // unused function
  //void copy(const arr2l& src_block, const arr2l& dest_array,
            //const tuple<int, int>& offset, int block_size) {
  //}
};

struct apsp_floyd_warshall_actor_state {
  
};

behavior apsp_floyd_warshall_actor_fun(stateful_actor<apsp_floyd_warshall_actor_state>* self, int my_block_id, int block_size, int graph_size, vector<vector<long>> int_graph_data) {
  return {
  
  };
}

void caf_main(actor_system& system, const config& /*cfg*/) {
}

CAF_MAIN()
