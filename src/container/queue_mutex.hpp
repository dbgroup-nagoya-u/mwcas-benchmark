// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <shared_mutex>

#include "queue.hpp"

namespace dbgroup::container
{
/**
 * @brief A class to implement a thread-safe queue by using C++ mutex library.
 *
 */
class QueueMutex : public Queue
{
 private:
  /**
   * @brief A class to represent nodes in a queue.
   *
   */
  struct Node {
    /// an element of a queue
    const T elem{};

    /// a previous node of a queue
    Node* next{nullptr};

    /// a mutex for thread-safe modification
    std::shared_mutex mtx{};
  };

  /// a dummy node to represent the tail of a queue
  Node front_;

  /// a dummy node to represent the head of a queue
  Node back_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  /**
   * @brief Construct a new QueueMutex object.
   *
   * The object uses C++ mutex library to perform thread-safe push/pop operations.
   */
  QueueMutex() : Queue{}, front_{T{}, &back_}, back_{T{}, &front_} {}

  ~QueueMutex()
  {
    while (!empty()) {
      pop();
    }
  }

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  T
  front() override
  {
    std::shared_lock<std::shared_mutex> guard{front_.mtx};

    return front_.next->elem;
  }

  T
  back() override
  {
    std::shared_lock<std::shared_mutex> guard{back_.mtx};

    return back_.next->elem;
  }

  void
  push(const T x) override
  {
    auto new_node = new Node{T{x}, &back_};

    while (true) {
      std::unique_lock<std::shared_mutex> guard{back_.mtx};

      auto old_node = back_.next;

      if (!old_node->mtx.try_lock()) {
        // if old_node cannot be locked, another thread performs push/pop
        continue;
      }

      back_.next = new_node;
      old_node->next = new_node;
      old_node->mtx.unlock();
      return;
    }
  }

  void
  pop() override
  {
    while (true) {
      std::unique_lock<std::shared_mutex> front_guard{front_.mtx};

      auto old_node = front_.next;
      if (old_node == &back_) {
        // if old_node is a back node, the queue is empty
        return;
      }

      if (!old_node->mtx.try_lock()) {
        // if old_node cannot be locked, another thread performs push/pop
        continue;
      }

      auto new_node = old_node->next;
      if (new_node != &back_) {
        // if new_node is not a back node, just swap the front
        front_.next = new_node;
      } else {
        // if new_node is a back node, it is required to swap the front/back nodes
        std::unique_lock<std::shared_mutex> back_guard{back_.mtx};
        front_.next = &back_;
        back_.next = &front_;
      }

      delete old_node;
      return;
    }
  }

  bool
  empty() override
  {
    std::shared_lock<std::shared_mutex> guard{front_.mtx};

    return front_.next == &back_;
  }

  bool
  IsValid() const override
  {
    auto prev_node = &front_;
    auto current_node = prev_node->next;

    while (current_node != &back_) {
      prev_node = current_node;
      current_node = current_node->next;
    }

    return current_node->next == prev_node;
  }
};

}  // namespace dbgroup::container
