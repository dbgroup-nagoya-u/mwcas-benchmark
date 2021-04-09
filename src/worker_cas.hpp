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
   * Public override functions
   *##############################################################################################*/

  void
  ReadMwCASField(const size_t index) override
  {
    const auto addr = shared_fields_ + index;
    reinterpret_cast<std::atomic_size_t *>(addr)->load(std::memory_order_relaxed);
  }

  void
  PerformMwCAS(const std::vector<size_t> &target_fields) override
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
