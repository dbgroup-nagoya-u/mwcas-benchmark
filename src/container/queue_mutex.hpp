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
  };

  /// a pointer to the tail of a queue
  Node* front_;

  /// a pointer to the head of a queue
  Node* back_;

  /// a mutex object for read/write locking
  std::shared_mutex mtx_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  /**
   * @brief Construct a new QueueMutex object.
   *
   * The object uses C++ mutex library to perform thread-safe push/pop operations.
   */
  QueueMutex() : Queue{}, mtx_{}
  {
    auto dummy_node = new Node{};
    front_ = dummy_node;
    back_ = dummy_node;
  }

  ~QueueMutex()
  {
    while (!empty()) {
      pop();
    }
    delete front_;
  }

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  T
  front() override
  {
    std::shared_lock<std::shared_mutex> guard{mtx_};

    return (front_->next == nullptr) ? T{} : front_->next->elem;
  }

  T
  back() override
  {
    std::shared_lock<std::shared_mutex> guard{mtx_};

    return back_->elem;
  }

  void
  push(const T x) override
  {
    auto new_node = new Node{T{x}, nullptr};

    std::unique_lock<std::shared_mutex> guard{mtx_};

    back_->next = new_node;
    back_ = new_node;
  }

  void
  pop() override
  {
    std::unique_lock<std::shared_mutex> guard{mtx_};

    auto head_node = front_->next;
    if (head_node == nullptr) {
      // if old_node is null, the queue is empty
      return;
    }

    delete front_;
    front_ = head_node;
  }

  bool
  empty() override
  {
    std::shared_lock<std::shared_mutex> guard{mtx_};

    return front_->next == nullptr;
  }

  bool
  IsValid() const override
  {
    auto prev_node = front_;
    auto current_node = prev_node->next;

    while (current_node != nullptr) {
      prev_node = current_node;
      current_node = current_node->next;
    }

    return prev_node == back_;
  }
};

}  // namespace dbgroup::container
