// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <chrono>
#include <random>
#include <utility>
#include <vector>

#include "common.hpp"
#include "mwcas/mwcas_manager.hpp"
#include "worker.hpp"

using dbgroup::atomic::MwCASManager;

class WorkerMwCAS : public Worker
{
 private:
  MwCASManager *manager_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  WorkerMwCAS(  //
      MwCASManager *mwcas_manager,
      const size_t shared_field_num,
      const size_t private_field_num,
      size_t *shared_fields,
      const size_t read_ratio,
      const size_t operation_counts)
      : Worker{shared_field_num, private_field_num, shared_fields, read_ratio, operation_counts},
        manager_{mwcas_manager}
  {
  }

  ~WorkerMwCAS() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  size_t
  ReadMwCASField() override
  {
    const auto start_time = std::chrono::high_resolution_clock::now();
    manager_->ReadMwCASField<size_t>(shared_fields_);
    const auto end_time = std::chrono::high_resolution_clock::now();

    const auto exec_time = end_time - start_time;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(exec_time).count();
  }

  size_t
  PerformMwCAS() override
  {
    std::chrono::_V2::system_clock::time_point start_time, end_time;
    dbgroup::atomic::mwcas::MwCASDescriptor *desc;

    start_time = std::chrono::high_resolution_clock::now();
    do {
      desc = manager_->CreateMwCASDescriptor();
      for (size_t index = 0; index < shared_num_; ++index) {
        auto addr = shared_fields_ + index;
        const auto old_val = manager_->ReadMwCASField<size_t>(addr);
        const auto new_val = old_val + 1;
        desc->AddEntry(addr, old_val, new_val);
      }
      for (size_t index = 0; index < private_num_; ++index) {
        auto addr = private_fields_ + index;
        const auto old_val = manager_->ReadMwCASField<size_t>(addr);
        const auto new_val = old_val + 1;
        desc->AddEntry(addr, old_val, new_val);
      }
    } while (!manager_->MwCAS(desc));
    end_time = std::chrono::high_resolution_clock::now();

    const auto exec_time = end_time - start_time;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(exec_time).count();
  }
};
