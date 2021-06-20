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

#pragma once

#include <atomic>
#include <chrono>
#include <random>
#include <utility>
#include <vector>

#include "common.hpp"
#include "worker_cas_base.hpp"

class WorkerSingleCAS : public WorkerCASBase
{
 protected:
  /*################################################################################################
   * Inherited utility functions
   *##############################################################################################*/

  void
  PerformMwCAS(const std::array<size_t, kMaxTargetNum> &target_fields) override
  {
    for (size_t i = 0; i < mwcas_target_num_; ++i) {
      const auto addr = target_fields_ + target_fields[i];
      auto target = reinterpret_cast<std::atomic_size_t *>(addr);
      auto old_val = target->load(std::memory_order_relaxed);
      size_t new_val;
      do {
        new_val = old_val + 1;
      } while (!target->compare_exchange_weak(old_val, new_val, std::memory_order_relaxed));
    }
  }

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  WorkerSingleCAS(  //
      size_t *target_fields,
      const size_t mwcas_target_num,
      const size_t operation_counts,
      ZipfGenerator &zipf_engine,
      const size_t random_seed = 0)
      : WorkerCASBase{target_fields, mwcas_target_num, operation_counts, zipf_engine, random_seed}
  {
  }
};
