// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <memory>

#include "deque.hpp"
#include "mwcas/mwcas.h"
#include "mwcas/mwcas_descriptor.hpp"
#include "pmwcas.h"

namespace dbgroup::container
{
using ::dbgroup::atomic::mwcas::MwCASDescriptor;
using ::dbgroup::atomic::mwcas::ReadMwCASField;

/**
 * @brief A class to implement a thread-safe deque by using Microsoft's PMwCAS library.
 *
 */
class DequePMwCAS : public Deque
{
 private:
  std::unique_ptr<pmwcas::DescriptorPool> desc_pool_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  DequePMwCAS() : Deque{}, desc_pool_{nullptr} {}

  /**
   * @brief Construct a new DequePMwCAS object.
   *
   * The object uses Microsoft's PMwCAS library to perform thread-safe push/pop operations.
   */
  explicit DequePMwCAS(const size_t thread_num) : Deque{}
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
  Front() override
  {
    auto epoch = desc_pool_->GetEpoch();
    epoch->Protect();

    const uintptr_t next_addr =
        reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(&front_.next)->GetValueProtected();
    const auto next_node = reinterpret_cast<Node *>(next_addr);

    epoch->Unprotect();
    return next_node->elem;
  }

  T
  Back() override
  {
    auto epoch = desc_pool_->GetEpoch();
    epoch->Protect();

    const uintptr_t prev_addr =
        reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(&back_.prev)->GetValueProtected();
    const auto prev_node = reinterpret_cast<Node *>(prev_addr);

    epoch->Unprotect();
    return prev_node->elem;
  }

  void
  PushFront(const T x) override
  {
    auto epoch = desc_pool_->GetEpoch();
    epoch->Protect();

    const uintptr_t front_addr = reinterpret_cast<uintptr_t>(&front_);
    uintptr_t *next_ptr = reinterpret_cast<uintptr_t *>(&(front_.next));
    uintptr_t old_addr =
        reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(next_ptr)->GetValueProtected();
    uintptr_t *prev_ptr =
        reinterpret_cast<uintptr_t *>(&(reinterpret_cast<Node *>(old_addr)->prev));

    auto new_node = new Node{T{x}, reinterpret_cast<Node *>(old_addr), &front_};
    const uintptr_t new_addr = reinterpret_cast<uintptr_t>(new_node);

    while (true) {
      auto desc = desc_pool_->AllocateDescriptor();

      desc->AddEntry(next_ptr, old_addr, new_addr);
      desc->AddEntry(prev_ptr, front_addr, new_addr);
      const bool success = desc->MwCAS();

      if (success) {
        break;
      } else {
        old_addr =
            reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(next_ptr)->GetValueProtected();
        new_node->next = reinterpret_cast<Node *>(old_addr);
        prev_ptr = reinterpret_cast<uintptr_t *>(&(reinterpret_cast<Node *>(old_addr)->prev));
      }
    }

    epoch->Unprotect();
  }

  void
  PushBack(const T x) override
  {
    auto epoch = desc_pool_->GetEpoch();
    epoch->Protect();

    const uintptr_t back_addr = reinterpret_cast<uintptr_t>(&back_);
    uintptr_t *prev_ptr = reinterpret_cast<uintptr_t *>(&(back_.prev));
    uintptr_t old_addr =
        reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(prev_ptr)->GetValueProtected();
    uintptr_t *next_ptr =
        reinterpret_cast<uintptr_t *>(&(reinterpret_cast<Node *>(old_addr)->next));

    auto new_node = new Node{T{x}, &back_, reinterpret_cast<Node *>(old_addr)};
    const uintptr_t new_addr = reinterpret_cast<uintptr_t>(new_node);

    while (true) {
      auto desc = desc_pool_->AllocateDescriptor();

      desc->AddEntry(next_ptr, back_addr, new_addr);
      desc->AddEntry(prev_ptr, old_addr, new_addr);
      const bool success = desc->MwCAS();

      if (success) {
        break;
      } else {
        old_addr =
            reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(prev_ptr)->GetValueProtected();
        new_node->prev = reinterpret_cast<Node *>(old_addr);
        next_ptr = reinterpret_cast<uintptr_t *>(&(reinterpret_cast<Node *>(old_addr)->next));
      }
    }

    epoch->Unprotect();
  }

  void
  PopFront() override
  {
    auto epoch = desc_pool_->GetEpoch();
    epoch->Protect();

    const uintptr_t front_addr = reinterpret_cast<uintptr_t>(&front_);
    uintptr_t *next_ptr = reinterpret_cast<uintptr_t *>(&(front_.next));
    uintptr_t old_addr =
        reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(next_ptr)->GetValueProtected();
    uintptr_t new_addr = reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(
                             &(reinterpret_cast<Node *>(old_addr)->next))
                             ->GetValueProtected();

    while (new_addr != 0) {  // if new_node is null, old_node is end of a deque
      uintptr_t *prev_ptr =
          reinterpret_cast<uintptr_t *>(&(reinterpret_cast<Node *>(new_addr)->prev));

      auto desc = desc_pool_->AllocateDescriptor();

      desc->AddEntry(next_ptr, old_addr, new_addr);
      desc->AddEntry(prev_ptr, old_addr, front_addr);
      const bool success = desc->MwCAS();

      if (success) {
        // delete reinterpret_cast<Node *>(old_addr);
        break;
      } else {
        old_addr =
            reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(next_ptr)->GetValueProtected();
        new_addr = reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(
                       &(reinterpret_cast<Node *>(old_addr)->next))
                       ->GetValueProtected();
      }
    }

    epoch->Unprotect();
  }

  void
  PopBack() override
  {
    auto epoch = desc_pool_->GetEpoch();
    epoch->Protect();

    const uintptr_t back_addr = reinterpret_cast<uintptr_t>(&back_);
    uintptr_t *prev_ptr = reinterpret_cast<uintptr_t *>(&(back_.prev));
    uintptr_t old_addr =
        reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(prev_ptr)->GetValueProtected();
    uintptr_t new_addr = reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(
                             &(reinterpret_cast<Node *>(old_addr)->prev))
                             ->GetValueProtected();

    while (new_addr != 0) {  // if new_node is null, old_node is end of a deque
      uintptr_t *next_ptr =
          reinterpret_cast<uintptr_t *>(&(reinterpret_cast<Node *>(new_addr)->next));

      auto desc = desc_pool_->AllocateDescriptor();

      desc->AddEntry(next_ptr, old_addr, back_addr);
      desc->AddEntry(prev_ptr, old_addr, new_addr);
      const bool success = desc->MwCAS();

      if (success) {
        // delete reinterpret_cast<Node *>(old_addr);
        break;
      } else {
        old_addr =
            reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(prev_ptr)->GetValueProtected();
        new_addr = reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(
                       &(reinterpret_cast<Node *>(old_addr)->prev))
                       ->GetValueProtected();
      }
    }

    epoch->Unprotect();
  }

  bool
  Empty() override
  {
    auto epoch = desc_pool_->GetEpoch();
    epoch->Protect();

    uintptr_t next_addr =
        reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(&(front_.next))->GetValueProtected();
    const uintptr_t next_next_addr = reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(
                                         &(reinterpret_cast<Node *>(next_addr)->next))
                                         ->GetValueProtected();

    epoch->Unprotect();
    return next_next_addr == 0;
  }
};

}  // namespace dbgroup::container
