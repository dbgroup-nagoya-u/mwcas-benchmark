/*
 * Copyright 2021 Database Group, Nagoya University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MWCAS_BENCHMARK_QUEUE_QUEUE_MWCAS_H
#define MWCAS_BENCHMARK_QUEUE_QUEUE_MWCAS_H

#include <optional>

// organization libraries
#include "memory/epoch_based_gc.hpp"
#include "mwcas/mwcas_descriptor.hpp"

namespace dbgroup::container
{

/**
 * @brief A class to implement a thread-safe queue by using our MwCAS library.
 *
 */
template <class T>
class QueueMwCAS
{
  /*####################################################################################
   * Type aliases
   *##################################################################################*/

  using MwCASDescriptor = ::dbgroup::atomic::mwcas::MwCASDescriptor;

 public:
  /*####################################################################################
   * Public constructors/destructors
   *##################################################################################*/

  /**
   * @brief Construct a new QueueMwCAS object.
   *
   * The object uses our MwCAS library to perform thread-safe push/pop operations.
   */
  QueueMwCAS() = default;

  /**
   * @brief Destroy the QueueMwCAS object
   *
   */
  ~QueueMwCAS()
  {
    while (!empty()) {
      pop();
    }
    delete front_.load(std::memory_order_relaxed);
  }

  /*####################################################################################
   * Public utility functions
   *##################################################################################*/

  void
  push(const T x)
  {
    [[maybe_unused]] const auto &guard = gc_.CreateEpochGuard();

    auto *page = gc_.template GetPageIfPossible<Node>();
    auto *new_node = (page) ? new (page) Node{x, nullptr} : new Node{x, nullptr};
    std::atomic_thread_fence(std::memory_order_release);
    while (true) {
      auto *back = MwCASDescriptor::Read<Node *>(&back_);

      MwCASDescriptor desc{};
      desc.AddMwCASTarget(&back_, back, new_node);
      desc.AddMwCASTarget(&(back->next), kNullNode, new_node);

      if (desc.MwCAS()) return;
    }
  }

  auto
  pop()  //
      -> std::optional<T>
  {
    [[maybe_unused]] const auto &guard = gc_.CreateEpochGuard();

    auto *front = front_.load(std::memory_order_relaxed);
    while (true) {
      auto *new_front = MwCASDescriptor::Read<Node *>(&(front->next));
      if (new_front == nullptr) return std::nullopt;
      std::atomic_thread_fence(std::memory_order_acquire);

      if (front_.compare_exchange_weak(front, new_front, std::memory_order_relaxed)) {
        gc_.AddGarbage(front);
        return new_front->elem;
      }
    }
  }

  auto
  empty()  //
      -> bool
  {
    [[maybe_unused]] const auto &guard = gc_.CreateEpochGuard();

    auto *front = front_.load(std::memory_order_relaxed);
    return MwCASDescriptor::Read<Node *>(&(front->next)) == nullptr;
  }

 private:
  /*####################################################################################
   * Internal classes
   *##################################################################################*/

  /**
   * @brief A class to represent nodes in a queue.
   *
   */
  struct Node {
    /// an element of a queue
    const T elem{};

    /// a previous node of a queue
    Node *next{nullptr};
  };

  /*####################################################################################
   * Internal constants
   *##################################################################################*/

  static constexpr size_t kGCInterval = 1000;

  static constexpr Node *kNullNode = nullptr;

  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  /// a pointer to the back (i.e., newest element) of a queue.
  Node *back_{new Node{}};

  /// a pointer to the front (i.e., oldest element) of a queue.
  std::atomic<Node *> front_{back_};

  /// a garbage collector for deleted nodes in a queue
  ::dbgroup::memory::EpochBasedGC<Node> gc_{kGCInterval};
};

}  // namespace dbgroup::container

#endif  // MWCAS_BENCHMARK_QUEUE_QUEUE_MWCAS_H
