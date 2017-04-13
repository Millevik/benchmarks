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


// 2D matrix class for the java/scala construct T[][]
template<class T>
struct matrix2d {
  matrix2d(size_t y, size_t x) 
      : width_(x)
      , v_(y * x) { // initalize all elements in the vector with T()
    // nop 
  }

  matrix2d() = default;
  matrix2d(const matrix2d&) = default;
  matrix2d(matrix2d&&) = default;
  matrix2d& operator=(const matrix2d&) = default;
  matrix2d& operator=(matrix2d&&) = default;
  
  inline T& operator()(size_t y, size_t x) {
    return v_[y * width_ + x];
  };

  inline const T& operator()(size_t y, size_t x) const {
    return v_[y * width_ + x];
  };

  inline std::vector<T> get_copy_of_line(size_t y) const {
    std::vector<T> result;
    result.reserve(width_);
    copy_n(begin(v_) + y, width_, back_inserter(result));
    return result;
  };
private:
  size_t width_;
  std::vector<T> v_;
};

class random_gen {
public:
  random_gen(long seed)
      : r_(seed) {
    // no
  }

  random_gen() = default;

  void set_seed(long seed) {
    r_ =  std::default_random_engine(seed);
  }

  int next_int(int exclusive_max=std::numeric_limits<int>::max()) {
    return r_() % (exclusive_max);
  }

  long long next_long() {
    return long_dis_(r_);
  }

  double next_double() {
    return double_dis_(r_);
  }

private:
  std::default_random_engine r_;
  std::uniform_int_distribution<long long> long_dis_;
  std::uniform_real_distribution<> double_dis_;
};

#endif // ERLANG_PATTERN_MATCHING_HPP
