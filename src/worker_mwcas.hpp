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

#include <chrono>
#include <random>
#include <utility>
#include <vector>

#include "common.hpp"
#include "mwcas/mwcas_descriptor.hpp"
#include "worker_cas_base.hpp"

class WorkerMwCAS : public WorkerCASBase
{
 protected:
  /*################################################################################################
   * Inherited utility functions
   *##############################################################################################*/

  void
  PerformMwCAS(const std::array<size_t, kMaxTargetNum>& target_fields) override
  {
    while (true) {
      dbgroup::atomic::mwcas::MwCASDescriptor desc;
      for (size_t i = 0; i < mwcas_target_num_; ++i) {
        const auto addr = target_fields_ + target_fields[i];
        const auto old_val = dbgroup::atomic::mwcas::ReadMwCASField<size_t>(addr);
        const auto new_val = old_val + 1;
        desc.AddMwCASTarget(addr, old_val, new_val);
      }
      if (desc.MwCAS()) break;
    }
  }

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  WorkerMwCAS(  //
      size_t* target_fields,
      const size_t mwcas_target_num,
      const size_t operation_counts,
      ZipfGenerator& zipf_engine,
      const size_t random_seed = 0)
      : WorkerCASBase{target_fields, mwcas_target_num, operation_counts, zipf_engine, random_seed}
  {
  }
};
