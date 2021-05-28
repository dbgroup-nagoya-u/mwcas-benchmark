// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <atomic>
#include <shared_mutex>

#include "deque.hpp"
#include "memory/manager/tls_based_memory_manager.hpp"

namespace dbgroup::container
{
/**
 * @brief A class to implement a thread-safe deque by using C++ CAS operations.
 *
 */
class DequeCAS : public Deque
{
  using GC_t = ::dbgroup::memory::manager::TLSBasedMemoryManager<Node>;

  static constexpr auto mo_relax = std::memory_order_relaxed;
  static constexpr size_t kGCInterval = 1E3;

 private:
  GC_t gc_;

  Node*
  LoadByCAS(void* addr)
  {
    return reinterpret_cast<std::atomic<Node*>*>(addr)->load(mo_relax);
  }

  bool
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
   * @brief Construct a new DequeCAS object.
   *
   * The object uses C++ CAS operations to perform thread-safe push/pop operations.
   */
  DequeCAS() : Deque{}, gc_{kGCInterval} {}

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  T
  Front() override
  {
    const auto guard = gc_.CreateEpochGuard();

    return LoadByCAS(&(front_.prev))->elem;
  }

  T
  Back() override
  {
    const auto guard = gc_.CreateEpochGuard();

    return LoadByCAS(&(back_.prev))->elem;
  }

  void
  PushFront(const T x) override
  {
    const auto guard = gc_.CreateEpochGuard();

    auto old_node = LoadByCAS(&(front_.prev));
    auto new_node = new Node{T{x}, nullptr, &front_};
    auto prev_node = &front_;

    // swap a backwars pointer
    while (true) {
      if (PerformCAS(&(old_node->prev), &prev_node, new_node)) {
        break;
      }
      if (prev_node != nullptr && prev_node != &front_) {
        // another thread has enqueued a new node
        old_node = prev_node;
        prev_node = &front_;
      } else if (prev_node == nullptr) {
        // another thread has dequeued a target node
        old_node = LoadByCAS(&(front_.prev));
        prev_node = &front_;
      }
      // if prev_node is not changed, CAS is failed due to its weak property
    }

    // swap a tail pointer
    while (true) {
      auto tmp_old = old_node;
      if (PerformCAS(&(front_.prev), &tmp_old, new_node)) {
        return;
      }
    }
  }

  void
  PushBack(const T x) override
  {
  }

  void
  PopFront() override
  {
  }

  void
  PopBack() override
  {
    const auto guard = gc_.CreateEpochGuard();

    auto old_node = LoadByCAS(&(back_.prev));

    while (true) {
      if (old_node == &front_) {
        // if old_node is a tail node, the queue is empty
        return;
      }

      auto new_node = LoadByCAS(&(old_node->prev));
      if (new_node != nullptr && new_node != &front_) {
        // if new_node is not a tail node, just swap a head
        if (PerformCAS(&(back_.prev), &old_node, new_node)) {
          break;
        }
      } else if (new_node == nullptr) {
        // if new_node is null, retry because another thread has popped it
        old_node = LoadByCAS(&(back_.prev));
      } else {  // new_node is a tail node, and it is required to swap head/tail nodes
        // first of all, swap the pointer of old_node to linearize push/pop operations
        auto tmp_node = &front_;
        if (!PerformCAS(&(old_node->prev), &tmp_node, nullptr)) {
          // if failed, retry because another thread has modified a queue
          old_node = LoadByCAS(&(back_.prev));
          continue;
        }

        // swap head/tail nodes
        tmp_node = old_node;
        while (!PerformCAS(&(back_.prev), &tmp_node, &front_)) {
          tmp_node = old_node;
        }
        while (!PerformCAS(&(front_.prev), &tmp_node, &back_)) {
          tmp_node = old_node;
        }
      }
    }

    gc_.AddGarbage(old_node);
  }

  bool
  Empty() override
  {
    const auto guard = gc_.CreateEpochGuard();

    return LoadByCAS(&(back_.prev)) == &front_;
  }
};

}  // namespace dbgroup::container
