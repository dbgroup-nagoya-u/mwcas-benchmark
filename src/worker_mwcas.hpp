// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <chrono>
#include <random>
#include <utility>
#include <vector>

#include "common.hpp"
#include "mwcas/mwcas_descriptor.hpp"
#include "worker.hpp"

class WorkerMwCAS : public Worker
{
 public:
  /*################################################################################################
   * Public override functions
   *##############################################################################################*/

  void
  ReadMwCASField(const size_t index) override
  {
    const auto addr = shared_fields_ + index;
    dbgroup::atomic::mwcas::ReadMwCASField<size_t>(addr);
  }

  void
  PerformMwCAS(const std::vector<size_t> &target_fields) override
  {
    while (true) {
      dbgroup::atomic::mwcas::MwCASDescriptor desc;
      for (auto &&index : target_fields) {
        const auto addr = shared_fields_ + index;
        const auto old_val = dbgroup::atomic::mwcas::ReadMwCASField<size_t>(addr);
        const auto new_val = old_val + 1;
        desc.AddMwCASTarget(addr, old_val, new_val);
      }
      if (desc.MwCAS()) break;
    }
  }
};
