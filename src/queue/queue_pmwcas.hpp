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

#ifndef MWCAS_BENCHMARK_QUEUE_QUEUE_PMWCAS_H
#define MWCAS_BENCHMARK_QUEUE_QUEUE_PMWCAS_H

#include <optional>

// external libraries
#include "mwcas/mwcas.h"
#include "pmwcas.h"

// organization libraries
#include "memory/epoch_based_gc.hpp"

namespace dbgroup::container
{

/**
 * @brief A class to implement a thread-safe queue by using our MwCAS library.
 *
 */
template <class T>
class QueuePMwCAS
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
   * @brief Construct a new QueuePMwCAS object.
   *
   * The object uses our MwCAS library to perform thread-safe push/pop operations.
   */
  explicit QueuePMwCAS(const size_t thread_num = 8)
  {
    pmwcas::InitLibrary(pmwcas::DefaultAllocator::Create, pmwcas::DefaultAllocator::Destroy,
                        pmwcas::LinuxEnvironment::Create, pmwcas::LinuxEnvironment::Destroy);
    desc_pool_ = std::make_unique<pmwcas::DescriptorPool>(static_cast<uint32_t>(8192 * thread_num),
                                                          static_cast<uint32_t>(thread_num));
  }

  /**
   * @brief Destroy the QueuePMwCAS object
   *
   */
  ~QueuePMwCAS()
  {
    pmwcas::UninitLibrary();

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
    while (true) {
      [[maybe_unused]] const pmwcas::EpochGuard epoch_guard{desc_pool_->GetEpoch()};

      auto *back = ReadNodeProtected(&back_);

      auto *desc = desc_pool_->AllocateDescriptor();
      desc->Initialize();
      AddEntry(desc, &back_, back, new_node);
      AddEntry(desc, &(back->next), kNullNode, new_node);

      if (desc->MwCAS()) return;
    }
  }

  auto
  pop()  //
      -> std::optional<T>
  {
    [[maybe_unused]] const auto &guard = gc_.CreateEpochGuard();

    auto *front = front_.load(std::memory_order_relaxed);
    while (true) {
      [[maybe_unused]] const pmwcas::EpochGuard epoch_guard{desc_pool_->GetEpoch()};

      auto *new_front = ReadNodeProtected(&(front->next));
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
    [[maybe_unused]] const pmwcas::EpochGuard epoch_guard{desc_pool_->GetEpoch()};

    auto *front = front_.load(std::memory_order_relaxed);
    return ReadNodeProtected(&(front->next)) == nullptr;
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
   * Internal utility functions
   *##################################################################################*/

  auto
  ReadNodeProtected(Node **addr)  //
      -> Node *
  {
    return reinterpret_cast<Node *>(
        reinterpret_cast<pmwcas::MwcTargetField<uintptr_t> *>(addr)->GetValueProtected());
  }

  void
  AddEntry(  //
      pmwcas::Descriptor *desc,
      Node **addr,
      Node *old_node,
      Node *new_node)
  {
    auto *addr_ptr = reinterpret_cast<uintptr_t *>(addr);
    auto old_addr = reinterpret_cast<uintptr_t>(old_node);
    auto new_addr = reinterpret_cast<uintptr_t>(new_node);

    desc->AddEntry(addr_ptr, old_addr, new_addr);
  }

  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  /// a pointer to the back (i.e., newest element) of a queue.
  Node *back_{new Node{}};

  /// a pointer to the front (i.e., oldest element) of a queue.
  std::atomic<Node *> front_{back_};

  /// a garbage collector for deleted nodes in a queue
  ::dbgroup::memory::EpochBasedGC<Node> gc_{kGCInterval};

  /// a pool for PMwCAS descriptors.
  std::unique_ptr<pmwcas::DescriptorPool> desc_pool_{nullptr};
};

}  // namespace dbgroup::container

#endif  // MWCAS_BENCHMARK_QUEUE_QUEUE_PMWCAS_H
