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
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#ifndef ERLANG_PATTERN_MATCHING_HPP
#define ERLANG_PATTERN_MATCHING_HPP

#include <vector>
#include <limits>
#include <random>

template<class T>
struct matrix2d {
  matrix2d(size_t y, size_t x) 
      : v_(y) { // initalize all elements in the vector with T()
    for (auto& v_line : v_) {
      v_line = std::vector<T>(x); 
    }
  }

  matrix2d() = default;
  matrix2d(const matrix2d&) = default;
  matrix2d(matrix2d&&) = default;
  matrix2d& operator=(const matrix2d&) = default;
  matrix2d& operator=(matrix2d&&) = default;
  
  inline T& operator()(size_t y, size_t x) {
    return v_[y][x];
  };

  inline const T& operator()(size_t y, size_t x) const {
    return v_[y][x];
  };
private:
  std::vector<std::vector<T>> v_;
};

class random_gen {
public:
  random_gen(long seed)
      : seed_(seed) 
      , r_(seed){
    // nop
  }

  random_gen() = default;

  void set_seed(long seed) {
    seed_ = seed;
  }

  int next_int(int exclusive_max = 65535) {
    return next_long() % exclusive_max;
  }

  long long next_long() {
    seed_ = ((seed_ * 1309) + 13849) & 65535;
    return seed_;
  }

  double next_double() {
    return double_dis_(r_);
  }

private:
  long seed_ = 74755; 
  std::default_random_engine r_;
  std::uniform_real_distribution<> double_dis_;
};
#endif // ERLANG_PATTERN_MATCHING_HPP
