// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "memory/manager/tls_based_memory_manager.hpp"
#include "mwcas/mwcas_descriptor.hpp"
#include "queue.hpp"

namespace dbgroup::container
{
using ::dbgroup::atomic::mwcas::MwCASDescriptor;
using ::dbgroup::atomic::mwcas::ReadMwCASField;

/**
 * @brief A class to implement a thread-safe queue by using our MwCAS library.
 *
 */
class QueueMwCAS : public Queue
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
  Node front_;

  /// a dummy node to represent the head of a queue
  Node back_;

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
  QueueMwCAS() : Queue{}, front_{T{}, &back_}, back_{T{}, &front_}, gc_{kGCInterval} {}

  /**
   * @brief Destroy the QueueMwCAS object
   *
   */
  ~QueueMwCAS()
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
    const auto guard = gc_.CreateEpochGuard();

    return ReadMwCASField<Node*>(&(front_.next))->elem;
  }

  T
  back() override
  {
    const auto guard = gc_.CreateEpochGuard();

    return ReadMwCASField<Node*>(&(back_.next))->elem;
  }

  void
  push(const T x) override
  {
    const auto guard = gc_.CreateEpochGuard();

    auto old_node = ReadMwCASField<Node*>(&(back_.next));
    auto new_node = new Node{T{x}, &back_};

    while (true) {
      MwCASDescriptor desc{};
      desc.AddMwCASTarget(&(back_.next), old_node, new_node);
      desc.AddMwCASTarget(&(old_node->next), &back_, new_node);

      if (desc.MwCAS()) {
        break;
      }
      old_node = ReadMwCASField<Node*>(&(back_.next));
    }
  }

  void
  pop() override
  {
    const auto guard = gc_.CreateEpochGuard();

    auto old_node = ReadMwCASField<Node*>(&(front_.next));

    while (true) {
      if (old_node == &back_) {
        // if old_node is a back node, the queue is empty
        return;
      }

      auto new_node = ReadMwCASField<Node*>(&(old_node->next));

      MwCASDescriptor desc{};
      if (new_node != &back_) {
        // if new_node is not a back node, just swap the front
        desc.AddMwCASTarget(&(front_.next), old_node, new_node);
      } else {
        // if new_node is a back node, it is required to swap the front/back nodes
        desc.AddMwCASTarget(&(front_.next), old_node, &back_);
        desc.AddMwCASTarget(&(back_.next), old_node, &front_);
      }

      if (desc.MwCAS()) {
        break;
      }
      old_node = ReadMwCASField<Node*>(&(front_.next));
    }

    gc_.AddGarbage(old_node);
  }

  bool
  empty() override
  {
    const auto guard = gc_.CreateEpochGuard();

    return ReadMwCASField<Node*>(&(front_.next)) == &back_;
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
