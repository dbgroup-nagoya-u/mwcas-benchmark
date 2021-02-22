// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <chrono>
#include <random>
#include <utility>
#include <vector>

#include "common.hpp"
#include "mwcas/mwcas.h"
#include "worker.hpp"

class WorkerPMwCAS : public Worker
{
 private:
  pmwcas::DescriptorPool* desc_pool_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  WorkerPMwCAS(  //
      pmwcas::DescriptorPool* desc_pool,
      const size_t shared_field_num,
      const size_t private_field_num,
      size_t* shared_fields,
      const size_t read_ratio,
      const size_t operation_counts)
      : Worker{shared_field_num, private_field_num, shared_fields, read_ratio, operation_counts},
        desc_pool_{desc_pool}
  {
  }

  ~WorkerPMwCAS() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  size_t
  ReadMwCASField() override
  {
    const auto start_time = std::chrono::high_resolution_clock::now();
    auto epoch = desc_pool_->GetEpoch();
    reinterpret_cast<pmwcas::MwcTargetField<size_t>*>(shared_fields_)->GetValue(epoch);
    const auto end_time = std::chrono::high_resolution_clock::now();

    const auto exec_time = end_time - start_time;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(exec_time).count();
  }

  size_t
  PerformMwCAS() override
  {
    // std::vector<MwCASEntry> entries;

    std::chrono::_V2::system_clock::time_point start_time, end_time;
    start_time = std::chrono::high_resolution_clock::now();

    auto desc = desc_pool_->AllocateDescriptor();
    auto epoch = desc_pool_->GetEpoch();
    epoch->Protect();
    do {
      for (size_t index = 0; index < shared_num_; ++index) {
        auto addr = shared_fields_ + index;
        const auto old_val =
            reinterpret_cast<pmwcas::MwcTargetField<size_t>*>(addr)->GetValueProtected();
        const auto new_val = old_val + 1;
        desc->AddEntry(addr, old_val, new_val);
      }
      for (size_t index = 0; index < private_num_; ++index) {
        auto addr = private_fields_ + index;
        const auto old_val =
            reinterpret_cast<pmwcas::MwcTargetField<size_t>*>(addr)->GetValueProtected();
        const auto new_val = old_val + 1;
        desc->AddEntry(addr, old_val, new_val);
      }
    } while (!desc->MwCAS());
    epoch->Unprotect();

    end_time = std::chrono::high_resolution_clock::now();
    const auto exec_time = end_time - start_time;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(exec_time).count();
  }
};
