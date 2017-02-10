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

  bitvector(size_t num_bits)
      : v_(get_block_idx(num_bits - 1) + 1, empty_value) 
      , num_bits_(num_bits)
      , padding_mask_(full_value << (num_bits % num_wrapper_bits))
  { } 

  inline void log_a_match(size_t bit_idx) {
      v_[get_block_idx(bit_idx)] |= get_block_mask(bit_idx);
  }

  inline bool get_match(size_t bit_idx) const {
    return v_[get_block_idx(bit_idx)] & get_block_mask(bit_idx);
  }

  bool all_matched() const {
    for (size_t i = 0; i < v_.size() - 1; ++i)
      if (v_[i] != full_value)
        return false; 
    if ((v_[v_.size()-1] | padding_mask_) != full_value)
      return false;
    return true;
  }
  
  std::string to_string() const {
    std::stringstream ss;
    ss << "bit size: " << num_bits_ << "\n";
    for (size_t i = 0; i < v_.size(); ++i)
      ss << "block idx(" << i << "):" << std::bitset<num_wrapper_bits>(v_[i])
         << "\n";
    ss << "padding_mask: " << std::bitset<num_wrapper_bits>(padding_mask_)
       << std::endl;
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

class vector_counter {
public:
  vector_counter() = default;
  vector_counter(const vector_counter&) = default;
  vector_counter(vector_counter&&) = default;
  vector_counter& operator = (vector_counter&&) = default;
  vector_counter& operator = (const vector_counter&) = default;

  vector_counter(size_t num_matches, size_t count_to)
      : v_(num_matches, 0)
      , count_to_(count_to)
  { } 

  inline void log_a_match(size_t idx) {
    ++v_[idx];
  }

  inline size_t get_match(size_t idx) {
    return v_[idx]; 
  }

  bool all_matched() const {
    for (auto& e : v_) {
      if (e != count_to_)
        return false;  
    } 
    return true;
  }

  std::string to_string() const {
    return std::string(); //implement if needed
  }
private:
  std::vector<size_t> v_;
  size_t count_to_;
};

template<class T, class U = std::vector<T>, class V = bitvector<>>
class erlang_pattern_matching {
public:
  using matched_list_t = U;

  template<class... Args>
  void foreach(const matched_list_t* match_list, Args... args) {
    match_list_ = match_list; // copy of pointer 
    init(std::forward<Args>(args)...);
  }

  template<class... Args>
  void foreach(const matched_list_t& match_list, Args... args) {
    match_list_ = match_list; // copy of data
    init(std::forward<Args>(args)...);
  }

  template<class... Args>
  void foreach(matched_list_t&& match_list, Args... args) {
    match_list_= std::move(match_list); // move of data
    init(std::forward<Args>(args)...);
  }
    
  bool match(const T& t) {
    if (failure_) 
      return false;
    // optimisation for find, matches are more or less done in the 
    // same as the match list, so if more than the half of matches ore done
    // we start searching for matches from the end of the mathc list
    size_t idx;
    if (recv_count < r(match_list_).size() / 2) {
      auto it = std::find(begin(r(match_list_)), end(r(match_list_)), t);
      if (it == end(r(match_list_))) {
        failure_ = true;
        return false;
      }
      idx = std::distance(begin(r(match_list_)), it);
    } else {
      auto it = std::find(r(match_list_).rbegin(), r(match_list_).rend(), t);
      if (it == match_list().rend()) {
        failure_ = true;
        return false;
      }
      idx =
        match_list().size() - 1 - std::distance(r(match_list_).rbegin(), it);
    }
    matches_.log_a_match(idx);
    ++recv_count; 
    if (recv_count == r(match_list_).size()) {
      if(matches_.all_matched()) {
        failure_ = true; // prevents from further false matches
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

  inline void restart() {
    init(); 
  }

  inline const matched_list_t& match_list() const {
    return match_list_;
  }

  inline matched_list_t& match_list() {
    return match_list_;
  }
private:
  template<class... Args>
  void init(Args... args) {
    matches_ = V(r(match_list_).size(), std::forward<Args>(args)...);
    recv_count = 0;
    matched_ = false;
    failure_ = false;
  }

  template<class X>
  inline const X& r(X& obj) const {
    return obj;
  }

  template<class X>
  inline const X& r(const X* obj) const {
    return *obj;
  }

  V matches_;
  matched_list_t match_list_;
  size_t recv_count = 0;
  bool matched_ = false;
  bool failure_ = true;
};

#endif // ERLANG_PATTERN_MATCHING_HPP
