#ifndef ERLANG_PATTERN_MATCHING_HPP
#define ERLANG_PATTERN_MATCHING_HPP

#include <vector>
#include <bitset>
#include <sstream>

template<class T = uint64_t>
class bitvector {
public:
  using wrapper_type = T;
  static const size_t num_wrapper_bits = sizeof(wrapper_type) * 8;
  static const wrapper_type full_value = ~static_cast<wrapper_type>(0);
  static const wrapper_type empty_value = static_cast<wrapper_type>(0);
  static const wrapper_type one_value = static_cast<wrapper_type>(1);

  bitvector() = default;
  bitvector(const bitvector&) = default;
  bitvector(bitvector&&) = default;
  bitvector& operator = (bitvector&&) = default;
  bitvector& operator = (const bitvector&) = default;

  bitvector(size_t num_bits, bool value)
      : v_(get_block_idx(num_bits - 1) + 1, value ? full_value : empty_value) 
      , num_bits_(num_bits)
      , padding_mask_(full_value << (num_bits % num_wrapper_bits))
  { } 

  inline void set(size_t bit_idx, bool v) {
    if (v)
      v_[get_block_idx(bit_idx)] |= get_block_mask(bit_idx);
    else
      v_[get_block_idx(bit_idx)] &= ~get_block_mask(bit_idx);
  }

  inline bool get(size_t bit_idx) const {
    return v_[get_block_idx(bit_idx)] & get_block_mask(bit_idx);
  }

  bool all_set() const {
    for (size_t i = 0; i < v_.size() - 1; ++i)
      if (v_[i] != full_value)
        return false; 
    if ((v_[v_.size()-1] | padding_mask_) != full_value)
      return false;
    return true;
  }
  
  inline size_t size() const {
    return num_bits_;
  }

  std::string to_string() const {
    std::stringstream ss;
    ss << "bit size: " << num_bits_ << "\n";
    for (size_t i = 0; i < v_.size(); ++i)
      ss << "block idx(" << i << "):" << std::bitset<num_wrapper_bits>(v_[i]) << "\n";
    ss << "padding_mask: " << std::bitset<num_wrapper_bits>(padding_mask_) << std::endl;
    return ss.str();
  }
private:
  // convert bit idx to wrapper type index
  inline size_t get_block_idx(size_t bit_idx) const {
    return bit_idx / num_wrapper_bits;
  }

  inline wrapper_type get_block_mask(size_t bit_idx) const {
    return one_value << (bit_idx % num_wrapper_bits);
  }

  std::vector<wrapper_type> v_;
  size_t num_bits_;
  wrapper_type padding_mask_;
};

template<class T>
class erlang_pattern_matching {
public:
  using matched_list_t = std::vector<T>;
  void foreach(const matched_list_t& match_list) {
    match_list_ = match_list; 
    init();
  }

  void foreach(matched_list_t&& match_list) {
    match_list_ = std::move(match_list); 
    init();
  }

  bool match(const T& t) {
    if (failure_) 
      return false;
    // optimisation for find, matches are more or less done in the 
    // same as the match list, so if more than the half of matches ore done
    // we start searching for matches from the end of the mathc list
    size_t idx;
    if (recv_count < match_list_.size() / 2) {
      auto it = std::find(begin(match_list_), end(match_list_), t);
      if (it == end(match_list_)) {
        failure_ = true;
        return false;
      }
      idx = std::distance(begin(match_list_), it);
    } else {
      auto it = std::find(match_list_.rbegin(), match_list_.rend(), t);
      if (it == match_list_.rend()) {
        failure_ = true;
        return false;
      }
      idx = match_list_.size() - 1 - std::distance(match_list_.rbegin(), it);
    }
    matches_.set(idx, true);
    ++recv_count; 
    if (recv_count == match_list_.size()) {
      if(matches_.all_set()) {
        matched_ = true;
      } else {
        failure_ = true;
      }
    }
    return matched_;
  }

  inline bool matched() const {
    return matched_; 
  }
private:
  void init() {
    matches_ = bitvector<>(match_list_.size(), false);
    recv_count = 0;
    matched_ = false;
    failure_ = false;
  }

  bitvector<> matches_;
  matched_list_t match_list_;
  size_t recv_count = 0;
  bool matched_ = false;
  bool failure_ = false;
};

#endif // ERLANG_PATTERN_MATCHING_HPP
