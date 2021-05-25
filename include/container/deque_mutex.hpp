// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <shared_mutex>

#include "deque.hpp"

namespace dbgroup::container
{
/**
 * @brief A class to implement a thread-safe deque by using our MwCAS library.
 *
 */
class DequeMutex : public Deque
{
 private:
  /// a mutex object to lock the head node
  std::shared_mutex front_mtx_;

  /// a mutex object to lock the tail node
  std::shared_mutex back_mtx_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  /**
   * @brief Construct a new DequeMutex object.
   *
   * The object uses our MwCAS library to perform thread-safe push/pop operations.
   */
  DequeMutex() : Deque{} {}

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  T
  Front() override
  {
    std::shared_lock<std::shared_mutex> guard{front_mtx_};
    return front_.next->elem;
  }

  T
  Back() override
  {
    std::shared_lock<std::shared_mutex> guard{back_mtx_};
    return back_.prev->elem;
  }

  void
  PushFront(const T x) override
  {
    std::unique_lock<std::shared_mutex> guard{front_mtx_};

    auto old_node = front_.next;
    auto new_node = new Node{T{x}, old_node, &front_};
    front_.next = new_node;
    old_node->prev = new_node;
  }

  void
  PushBack(const T x) override
  {
    std::unique_lock<std::shared_mutex> guard{back_mtx_};

    auto old_node = back_.prev;
    auto new_node = new Node{T{x}, &back_, old_node};
    old_node->next = new_node;
    back_.prev = new_node;
  }

  void
  PopFront() override
  {
    std::unique_lock<std::shared_mutex> guard{front_mtx_};

    auto old_node = front_.next;
    auto new_node = old_node->next;

    if (new_node != nullptr) {  // if new_node is null, old_node is end of a deque
      front_.next = new_node;
      new_node->prev = &front_;
      delete old_node;
    }
  }

  void
  PopBack() override
  {
    std::unique_lock<std::shared_mutex> guard{back_mtx_};

    auto old_node = back_.prev;
    auto new_node = old_node->prev;

    if (new_node != nullptr) {  // if new_node is null, old_node is end of a deque
      new_node->next = &back_;
      back_.prev = new_node;
      delete old_node;
    }
  }

  bool
  Empty() override
  {
    std::shared_lock<std::shared_mutex> guard{front_mtx_};
    return front_.next->next == nullptr;
  }
};

}  // namespace dbgroup::container
