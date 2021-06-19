// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "common.hpp"
#include "memory/manager/tls_based_memory_manager.hpp"
#include "mwcas/mwcas_descriptor.hpp"

namespace dbgroup::container
{
using ::dbgroup::atomic::mwcas::MwCASDescriptor;
using ::dbgroup::atomic::mwcas::ReadMwCASField;

/**
 * @brief A class to implement a thread-safe queue by using our MwCAS library.
 *
 */
class QueueMwCAS
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

  /// a dummy node to represent the tail of a queue
  Node* front_;

  /// a dummy node to represent the head of a queue
  Node* back_;

  /// a garbage collector for deleted nodes in a queue
  ::dbgroup::memory::manager::TLSBasedMemoryManager<Node> gc_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  /**
   * @brief Construct a new QueueMwCAS object.
   *
   * The object uses our MwCAS library to perform thread-safe push/pop operations.
   */
  QueueMwCAS() : gc_{kGCInterval}
  {
    auto dummy_node = new Node{};
    front_ = dummy_node;
    back_ = dummy_node;
  }

  /**
   * @brief Destroy the QueueMwCAS object
   *
   */
  ~QueueMwCAS()
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

    const auto dummy_node = ReadMwCASField<Node*>(&front_);
    const auto head_node = ReadMwCASField<Node*>(&(dummy_node->next));
    return (head_node == nullptr) ? T{} : head_node->elem;
  }

  T
  back()
  {
    const auto guard = gc_.CreateEpochGuard();

    return ReadMwCASField<Node*>(&back_)->elem;
  }

  void
  push(const T x)
  {
    const auto guard = gc_.CreateEpochGuard();

    auto new_node = new Node{x, nullptr};
    while (true) {
      auto tail_node = ReadMwCASField<Node*>(&back_);

      MwCASDescriptor desc{};
      desc.AddMwCASTarget(&back_, tail_node, new_node);
      desc.AddMwCASTarget(&(tail_node->next), static_cast<Node*>(nullptr), new_node);

      if (desc.MwCAS()) return;
    }
  }

  void
  pop()
  {
    const auto guard = gc_.CreateEpochGuard();

    while (true) {
      auto dummy_node = ReadMwCASField<Node*>(&front_);
      auto head_node = ReadMwCASField<Node*>(&(dummy_node->next));

      if (head_node == nullptr) {
        // if new_node is null, the queue is empty
        return;
      }

      MwCASDescriptor desc{};
      desc.AddMwCASTarget(&front_, dummy_node, head_node);

      if (desc.MwCAS()) {
        gc_.AddGarbage(dummy_node);
        return;
      }
    }
  }

  bool
  empty()
  {
    const auto guard = gc_.CreateEpochGuard();

    auto dummy_node = ReadMwCASField<Node*>(&front_);
    return ReadMwCASField<Node*>(&(dummy_node->next)) == nullptr;
  }

  bool
  IsValid() const
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
