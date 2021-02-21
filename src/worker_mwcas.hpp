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

using dbgroup::atomic::mwcas::MwCASEntry;
using dbgroup::atomic::mwcas::MwCASManager;

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
      const size_t private_field_num,
      const std::vector<size_t *> shared_fields,
      const size_t read_ratio,
      const size_t operation_counts)
      : Worker{private_field_num, shared_fields, read_ratio, operation_counts},
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
    const auto start_time = std::chrono::system_clock::now();
    manager_->ReadMwCASField<size_t>(shared_fields_[0]);
    const auto end_time = std::chrono::system_clock::now();

    const auto exec_time = end_time - start_time;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(exec_time).count();
  }

  size_t
  PerformMwCAS() override
  {
    std::vector<MwCASEntry> entries;

    const auto start_time = std::chrono::system_clock::now();

    do {
      entries = std::vector<MwCASEntry>{};
      for (size_t index = 0; index < shared_fields_.size(); ++index) {
        const auto old_val = manager_->ReadMwCASField<size_t>(shared_fields_[index]);
        const auto new_val = old_val + 1;
        entries.emplace_back(shared_fields_[index], old_val, new_val);
      }
      for (size_t index = 0; index < private_fields_.size(); ++index) {
        const auto old_val = manager_->ReadMwCASField<size_t>(&private_fields_[index]);
        const auto new_val = old_val + 1;
        entries.emplace_back(&private_fields_[index], old_val, new_val);
      }
    } while (manager_->MwCAS(std::move(entries)));

    const auto end_time = std::chrono::system_clock::now();

    const auto exec_time = end_time - start_time;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(exec_time).count();
  }
};
