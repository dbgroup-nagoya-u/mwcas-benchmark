// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <atomic>
#include <chrono>
#include <random>
#include <utility>
#include <vector>

#include "common.hpp"
#include "worker.hpp"

class WorkerSingleCAS : public Worker
{
 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  WorkerSingleCAS(  //
      size_t *shared_fields,
      const size_t shared_field_num,
      const size_t target_field_num,
      const size_t read_ratio,
      const size_t operation_counts,
      const size_t loop_num,
      const size_t random_seed = 0)
      : Worker{shared_fields,    shared_field_num, target_field_num, read_ratio,
               operation_counts, loop_num,         random_seed}
  {
  }

  /*################################################################################################
   * Public override functions
   *##############################################################################################*/

  void
  ReadMwCASField(const size_t index) override
  {
    const auto addr = shared_fields_ + index;
    reinterpret_cast<std::atomic_size_t *>(addr)->load(std::memory_order_relaxed);
  }

  void
  PerformMwCAS(const std::array<size_t, kMaxTargetNum> &target_fields) override
  {
    const auto addr = shared_fields_ + target_fields.front();
    auto target = reinterpret_cast<std::atomic_size_t *>(addr);
    auto old_val = target->load(std::memory_order_relaxed);
    while (true) {
      const auto new_val = old_val + 1;
      if (target->compare_exchange_weak(old_val, new_val, std::memory_order_relaxed)) break;
    }
  }
};
