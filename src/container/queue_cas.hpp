// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <atomic>
#include <shared_mutex>

#include "memory/manager/tls_based_memory_manager.hpp"
#include "queue.hpp"

namespace dbgroup::container
{
/**
 * @brief A class to implement a thread-safe queue by using C++ CAS operations.
 *
 */
class QueueCAS : public Queue
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
  Node back_;

  /// a dummy node to represent the head of a queue
  Node front_;

  /// a garbage collector for deleted nodes in a queue
  ::dbgroup::memory::manager::TLSBasedMemoryManager<Node> gc_;

  /**
   * @brief Load a node pointer from the specified address atomically.
   *
   * @param addr a target address
   * @return Node* a loaded node pointer
   */
  static Node*
  LoadByCAS(void* addr)
  {
    return reinterpret_cast<std::atomic<Node*>*>(addr)->load(mo_relax);
  }

  /**
   * @brief Perform a CAS operation with the specified parameters.
   *
   * @param addr a target address
   * @param old_node an expected node address and modified when failed
   * @param new_node an desired node address
   * @retval true if CAS succeeded
   * @retval false if CAS failed
   */
  static bool
  PerformCAS(void* addr, Node** old_node, Node* new_node)
  {
    return reinterpret_cast<std::atomic<Node*>*>(addr)->compare_exchange_weak(*old_node, new_node,
                                                                              mo_relax);
  }

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  /**
   * @brief Construct a new QueueCAS object.
   *
   * The object uses C++ CAS operations to perform thread-safe push/pop operations.
   */
  QueueCAS() : Queue{}, back_{T{}, &front_}, front_{T{}, &back_}, gc_{kGCInterval} {}

  /**
   * @brief Destroy the QueueCAS object
   *
   */
  ~QueueCAS()
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

    return LoadByCAS(&(front_.next))->elem;
  }

  T
  back() override
  {
    const auto guard = gc_.CreateEpochGuard();

    return LoadByCAS(&(back_.next))->elem;
  }

  void
  push(const T x) override
  {
    const auto guard = gc_.CreateEpochGuard();

    auto old_node = LoadByCAS(&(back_.next));
    auto new_node = new Node{T{x}, &back_};
    auto prev_node = &back_;

    // swap a backwars pointer
    while (true) {
      if (PerformCAS(&(old_node->next), &prev_node, new_node)) {
        break;
      }
      if (prev_node != nullptr && prev_node != &back_) {
        // another thread has pushed a new node
        old_node = prev_node;
        prev_node = &back_;
      } else if (prev_node == nullptr) {
        // another thread has popped a target node
        old_node = LoadByCAS(&(back_.next));
        prev_node = &back_;
      }
      // if prev_node is not changed, CAS is failed due to its weak property
    }

    // swap a tail pointer
    while (true) {
      auto tmp_old = old_node;
      if (PerformCAS(&(back_.next), &tmp_old, new_node)) {
        return;
      }
    }
  }

  void
  pop() override
  {
    const auto guard = gc_.CreateEpochGuard();

    auto old_node = LoadByCAS(&(front_.next));

    while (true) {
      if (old_node == &back_) {
        // if old_node is a tail node, the queue is empty
        return;
      }

      auto new_node = LoadByCAS(&(old_node->next));
      if (new_node != nullptr && new_node != &back_) {
        // if new_node is not a tail node, just swap a head
        if (PerformCAS(&(front_.next), &old_node, new_node)) {
          break;
        }
      } else if (new_node == nullptr) {
        // if new_node is null, retry because another thread has popped it
        old_node = LoadByCAS(&(front_.next));
      } else {  // new_node is a tail node, and it is required to swap head/tail nodes
        // first of all, swap the pointer of old_node to linearize push/pop operations
        auto tmp_node = &back_;
        if (!PerformCAS(&(old_node->next), &tmp_node, nullptr)) {
          // if failed, retry because another thread has modified a queue
          old_node = LoadByCAS(&(front_.next));
          continue;
        }

        // swap head/tail nodes
        tmp_node = old_node;
        while (!PerformCAS(&(front_.next), &tmp_node, &back_)) {
          tmp_node = old_node;
        }
        while (!PerformCAS(&(back_.next), &tmp_node, &front_)) {
          tmp_node = old_node;
        }
      }
    }

    gc_.AddGarbage(old_node);
  }

  bool
  empty() override
  {
    const auto guard = gc_.CreateEpochGuard();

    return LoadByCAS(&(front_.next)) == &back_;
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
