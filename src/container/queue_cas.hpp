// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <atomic>
#include <shared_mutex>

#include "common.hpp"
#include "memory/manager/tls_based_memory_manager.hpp"

namespace dbgroup::container
{
/**
 * @brief A class to implement a thread-safe queue by using C++ CAS operations.
 *
 */
class QueueCAS
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
    std::atomic<Node*> next{nullptr};
  };

  /// a dummy node to represent the tail of a queue
  std::atomic<Node*> back_;

  /// a dummy node to represent the head of a queue
  std::atomic<Node*> front_;

  /// a garbage collector for deleted nodes in a queue
  ::dbgroup::memory::manager::TLSBasedMemoryManager<Node> gc_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  /**
   * @brief Construct a new QueueCAS object.
   *
   * The object uses C++ CAS operations to perform thread-safe push/pop operations.
   */
  QueueCAS() : gc_{kGCInterval}
  {
    auto dummy_node = new Node{};
    front_ = dummy_node;
    back_ = dummy_node;
  }

  /**
   * @brief Destroy the QueueCAS object
   *
   */
  ~QueueCAS()
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
  front()
  {
    const auto guard = gc_.CreateEpochGuard();

    const auto dummy_node = front_.load(mo_relax);
    const auto head_node = dummy_node->next.load(mo_relax);
    return (head_node == nullptr) ? T{} : head_node->elem;
  }

  T
  back()
  {
    const auto guard = gc_.CreateEpochGuard();

    return back_.load(mo_relax)->elem;
  }

  void
  push(const T x)
  {
    const auto guard = gc_.CreateEpochGuard();

    auto new_node = new Node{x, nullptr};

    while (true) {
      auto tail_node = back_.load(mo_relax);
      auto null_node = tail_node->next.load(mo_relax);

      if (null_node != nullptr) {
        // if tail_node has the next node, another thread is pushing an element concurrently
        back_.compare_exchange_weak(tail_node, null_node, mo_relax);
        continue;
      }

      if (!tail_node->next.compare_exchange_weak(null_node, new_node, mo_relax)) {
        // if CAS failed, another thread is pushing an element concurrently
        continue;
      }

      while (!back_.compare_exchange_weak(tail_node, new_node, mo_relax)) {
        if (tail_node == new_node) {
          break;
        }
      }

      return;
    }
  }

  void
  pop()
  {
    const auto guard = gc_.CreateEpochGuard();

    while (true) {
      auto dummy_node = front_.load(mo_relax);
      auto head_node = dummy_node->next.load(mo_relax);

      if (head_node == nullptr) {
        // if head_node is null, the queue is empty
        return;
      }

      auto tail_node = back_.load(mo_relax);
      if (tail_node == dummy_node) {
        // the queue is not empty but has the same head/tail node,
        // another thread is pushing an element concurrently
        auto current_tail = tail_node->next.load(mo_relax);
        back_.compare_exchange_weak(dummy_node, current_tail, mo_relax);
        continue;
      }

      if (front_.compare_exchange_weak(dummy_node, head_node, mo_relax)) {
        gc_.AddGarbage(dummy_node);
        return;
      }
    }
  }

  bool
  empty()
  {
    const auto guard = gc_.CreateEpochGuard();

    const auto dummy_node = front_.load(mo_relax);
    return dummy_node->next.load(mo_relax) == nullptr;
  }

  bool
  IsValid() const
  {
    auto prev_node = front_.load(mo_relax);
    auto current_node = prev_node->next.load(mo_relax);

    while (current_node != nullptr) {
      prev_node = current_node;
      current_node = current_node->next.load(mo_relax);
    }

    return prev_node == back_.load(mo_relax);
  }
};

}  // namespace dbgroup::container
