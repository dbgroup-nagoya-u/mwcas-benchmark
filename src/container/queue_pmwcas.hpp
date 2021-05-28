// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <memory>

#include "memory/manager/tls_based_memory_manager.hpp"
#include "mwcas/mwcas.h"
#include "pmwcas.h"
#include "queue.hpp"

namespace dbgroup::container
{
/**
 * @brief A class to implement a thread-safe queue by using Wang's PMwCAS library.
 *
 */
class QueuePMwCAS : public Queue
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
    Node *next{nullptr};
  };

  /// a dummy node to represent the tail of a queue
  Node front_;

  /// a dummy node to represent the head of a queue
  Node back_;

  /// a garbage collector for deleted nodes in a queue
  ::dbgroup::memory::manager::TLSBasedMemoryManager<Node> gc_;

  /// a pool for PMwCAS descriptors
  std::unique_ptr<pmwcas::DescriptorPool> desc_pool_;

  Node *
  ReadNodeAddrProtected(Node **addr)
  {
    return reinterpret_cast<Node *>(
        reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(addr)->GetValueProtected());
  }

  void
  AddEntry(  //
      pmwcas::Descriptor *desc,
      Node **addr,
      Node *old_node,
      Node *new_node)
  {
    auto addr_ptr = reinterpret_cast<uintptr_t *>(addr);
    auto old_addr = reinterpret_cast<uintptr_t>(old_node);
    auto new_addr = reinterpret_cast<uintptr_t>(new_node);

    desc->AddEntry(addr_ptr, old_addr, new_addr);
  }

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  QueuePMwCAS()
      : Queue{}, front_{T{}, &back_}, back_{T{}, &front_}, gc_{kGCInterval}, desc_pool_{nullptr}
  {
  }

  /**
   * @brief Construct a new QueuePMwCAS object.
   *
   * The object uses Wang's PMwCAS library to perform thread-safe push/pop operations.
   */
  explicit QueuePMwCAS(const size_t thread_num)
      : Queue{}, front_{T{}, &back_}, back_{T{}, &front_}, gc_{kGCInterval}
  {
    pmwcas::InitLibrary(pmwcas::DefaultAllocator::Create, pmwcas::DefaultAllocator::Destroy,
                        pmwcas::LinuxEnvironment::Create, pmwcas::LinuxEnvironment::Destroy);
    desc_pool_ = std::make_unique<pmwcas::DescriptorPool>(static_cast<uint32_t>(8192 * thread_num),
                                                          static_cast<uint32_t>(thread_num));
  }

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  T
  front() override
  {
    const auto gc_guard = gc_.CreateEpochGuard();
    const pmwcas::EpochGuard epoch_guard{desc_pool_->GetEpoch()};

    return ReadNodeAddrProtected(&(front_.next))->elem;
  }

  T
  back() override
  {
    const auto gc_guard = gc_.CreateEpochGuard();
    const pmwcas::EpochGuard epoch_guard{desc_pool_->GetEpoch()};

    return ReadNodeAddrProtected(&(back_.next))->elem;
  }

  void
  push(const T x) override
  {
    const auto gc_guard = gc_.CreateEpochGuard();
    const pmwcas::EpochGuard epoch_guard{desc_pool_->GetEpoch()};

    auto old_node = ReadNodeAddrProtected(&(back_.next));
    auto new_node = new Node{T{x}, &back_};

    while (true) {
      auto desc = desc_pool_->AllocateDescriptor();

      AddEntry(desc, &(back_.next), old_node, new_node);
      AddEntry(desc, &(old_node->next), &back_, new_node);

      if (desc->MwCAS()) {
        break;
      }
      old_node = ReadNodeAddrProtected(&(back_.next));
    }
  }

  void
  pop() override
  {
    const auto gc_guard = gc_.CreateEpochGuard();
    const pmwcas::EpochGuard epoch_guard{desc_pool_->GetEpoch()};

    auto old_node = ReadNodeAddrProtected(&(front_.next));

    while (true) {
      if (old_node == &back_) {
        // if old_node is a back node, the queue is empty
        return;
      }

      auto new_node = ReadNodeAddrProtected(&(old_node->next));

      auto desc = desc_pool_->AllocateDescriptor();
      if (new_node != &back_) {
        // if new_node is not a back node, just swap the front
        AddEntry(desc, &(front_.next), old_node, new_node);
      } else {
        // if new_node is a back node, it is required to swap the front/back nodes
        AddEntry(desc, &(front_.next), old_node, &back_);
        AddEntry(desc, &(back_.next), old_node, &front_);
      }

      if (desc->MwCAS()) {
        break;
      }
      old_node = ReadNodeAddrProtected(&(front_.next));
    }

    gc_.AddGarbage(old_node);
  }

  bool
  empty() override
  {
    const auto gc_guard = gc_.CreateEpochGuard();
    const pmwcas::EpochGuard epoch_guard{desc_pool_->GetEpoch()};

    return ReadNodeAddrProtected(&(front_.next)) == &back_;
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
