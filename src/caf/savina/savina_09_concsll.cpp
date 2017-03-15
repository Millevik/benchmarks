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
#include <random>
#include <unordered_map>
#include <limits>
#include <sstream>

#include "caf/all.hpp"

using namespace std;
using namespace caf;

#define CAF_ALLOW_UNSAFE_MESSAGE_TYPE(type_name)                               \
  namespace caf {                                                              \
  template <>                                                                  \
  struct allowed_unsafe_message_type<type_name> : std::true_type {};           \
  }

//using end_work_msg_atom = atom_constant<atom("endwork")>;

class config : public actor_system_config {
public:
  int num_entitites = 20;
  int num_msgs_per_worker = 8000;
  static int write_percentage; // = 10;
  static int size_percentage; // = 1;
  
  config() {
    opt_group{custom_options_, "global"}
    .add(num_entitites, "eee,e", "number of entities")
    .add(num_msgs_per_worker, "mmm,m", "number of messges per worker")
    .add(write_percentage, "www,w", "write percent")
    .add(size_percentage, "sss,s", "size percentage");

    if (write_percentage >= 50) {
      throw "Write rate must be less than 50!";
    }
    if ((2 * write_percentage + size_percentage) >= 100) {
      throw "(2 * write-rate) + sum-rate must be less than 100!";
    }
  }
};
int config::write_percentage = 10;
int config::size_percentage = 1;

struct write_msg {
  //actor sender;
  int value;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(write_msg);

struct contains_msg {
  //actor sender;
  int value;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(contains_msg);

struct size_msg {
  //actor sender; 
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(size_msg);

struct result_msg {
  //actor sender;
  int value;
};
constexpr auto do_work_msg = result_msg{-1};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(result_msg);

//using do_work_msg_atom = atom_constant<atom("dowork")>;
using end_work_msg_atom = atom_constant<atom("endwork")>;

template<class T>
int compare_to(const T& left, const T& right);

template<> 
int compare_to<int>(const int& left, const int& right) {
  return  left - right;
}

template<class T>
class sorted_linked_list {
private:
  struct node {
    T item; 
    unique_ptr<node> next;

    node(const T& i) 
        : item(i)
        , next(nullptr) {
      // nop  
    }
  };

  unique_ptr<node> head;
  node* iterator;

public:
  sorted_linked_list() 
      : head(nullptr)
      , iterator(nullptr) {
    // nop    
  }

  bool is_empty() const {
    return head == nullptr; 
  }

  void add(const T& item) {
    unique_ptr<node> new_node(new node(item));
    if (head == nullptr) {
      head = move(new_node);
    } else if (compare_to(item, head->item) < 0) {
      new_node->next = move(head);
      head = move(new_node);
    } else {
      node* after = head->next.get(); 
      node* before = head.get();
      while (after != nullptr) {
        if (compare_to(item, after->item) < 0) {
          break;
        }
        before = after;
        after = after->next.get();
      }
      new_node->next = move(before->next);
      before->next = move(new_node); 
    }
  }

  bool contains(const T& item) const {
    node* n = head.get(); 
    while(n != nullptr) {
      if (compare_to(item, n->item) == 0) {
        return true; 
      } 
      n = n->next.get();
    }
    return false;
  }

  string to_string() const {
    stringstream s;
    node* n = head.get();
    while (n != nullptr) {
      s << n->item << " ";
      n = n->next.get();
    }
    return s.str();
  }

  T* next() {
    if (iterator != nullptr) {
      node* n = iterator;
      iterator = iterator->next.get();
      return &n->item;
    } else {
      return nullptr; 
    } 
  }

  void reset() {
    iterator = head.get(); 
  }

  int size() const {
    int r = 0;
    node* n = head.get();
    while (n != nullptr) {
      ++r; 
      n = n->next.get();
    }
    return r;
  }
};

behavior worker_fun(event_based_actor* self, actor master, actor sorted_list,
                    int id, int num_msgs_per_worker) {

  auto write_percent = config::write_percentage;
  auto size_percent = config::size_percentage;
  int msg_count = 0;
  default_random_engine random(id + num_msgs_per_worker + write_percent);
  auto next_int = [](default_random_engine& r,
                     int inclusive_max =
                       std::numeric_limits<int>::max()) -> int {
    return r() % (inclusive_max +1);
  };
  return {
    [=](result_msg&) mutable {
      ++msg_count; 
      if (msg_count <= num_msgs_per_worker) {
        int an_int = next_int(random, 100); 
        if (an_int < size_percent) {
          self->send(sorted_list, size_msg{});
        } else if (an_int < (size_percent + write_percent)) {
          self->send(sorted_list, write_msg{next_int(random)});
        } else {
          self->send(sorted_list, contains_msg{next_int(random)});
        }
      } else {
        self->send(master, end_work_msg_atom::value); 
      }
    }   
  };
}

behavior sorted_list_fun(stateful_actor<sorted_linked_list<int>>* self) {
  return {
    [=](write_msg& write_message) {
      auto& data_list = self->state;
      auto value = write_message.value; 
      data_list.add(value); 
      return result_msg{value};
    },
    [=](contains_msg& contains_message) {
      auto& data_list = self->state;
      auto value = contains_message.value;
      auto result = data_list.contains(value) ? 1 : 0;
      return result_msg{result};
    },
    [=](size_msg&) {
      auto& data_list = self->state;
      auto value = data_list.size();
      return result_msg{value};
    },
    [=](end_work_msg_atom) {
      self->quit(); 
    }
  };
}

behavior master_fun(event_based_actor* self, int num_workers,
                    int num_msgs_per_worker) {
  vector<actor> workers;
  auto sorted_list = self->spawn(sorted_list_fun);
  int num_workers_terminated = 0;
  // onPostStart()
  workers.reserve(num_workers);
  for (int i = 0; i < num_workers; ++i) {
    workers.emplace_back(self->spawn(worker_fun, actor_cast<actor>(self),
                                     sorted_list, i, num_msgs_per_worker));
    self->send(workers[i], do_work_msg);
  }
  return {
    [=](end_work_msg_atom) mutable {
      ++num_workers_terminated; 
      if (num_workers_terminated == num_workers) {
        self->send(sorted_list, end_work_msg_atom::value); 
        self->quit();
      }
    }
  };
}

void caf_main(actor_system& system, const config& cfg) {
  auto num_workers = cfg.num_entitites;
  auto num_msgs_per_worker = cfg.num_msgs_per_worker;
  auto master = system.spawn(master_fun, num_workers, num_msgs_per_worker);
}

CAF_MAIN()
