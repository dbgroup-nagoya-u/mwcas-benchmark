// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <random>
#include <utility>
#include <vector>

#include "common.hpp"

class Worker
{
 private:
  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  size_t read_ratio_;

  size_t operation_counts_;

  std::vector<std::pair<OperationType, size_t>> exec_times_nano_;

 protected:
  /*################################################################################################
   * Inherited member variables
   *##############################################################################################*/

  std::vector<size_t *> shared_fields_;

  std::vector<size_t> private_fields_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  Worker(  //
      const size_t private_field_num,
      const std::vector<size_t *> shared_fields,
      const size_t read_ratio,
      const size_t operation_counts)
      : read_ratio_{read_ratio}, operation_counts_{operation_counts}
  {
    exec_times_nano_.reserve(operation_counts_);
    shared_fields_.insert(shared_fields_.begin(), shared_fields.begin(), shared_fields.end());
    for (size_t count = 0; count < private_field_num; ++count) {
      private_fields_.emplace_back(0);
    }
  }

  ~Worker() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  virtual size_t ReadMwCASField();

  virtual size_t PerformMwCAS();

  void
  Run()
  {
    std::random_device seed_gen;
    std::mt19937_64 rand_engine{seed_gen};

    for (size_t count = 0; count < operation_counts_; ++count) {
      const auto rand_val = rand_engine() % 100;
      if (rand_val < read_ratio_) {
        // perform a MwCAS read operation
        const auto exec_time = ReadMwCASField();
        exec_times_nano_.emplace_back(OperationType::kRead, exec_time);
      } else {
        // perform a MwCAS operation
        const auto exec_time = PerformMwCAS();
        exec_times_nano_.emplace_back(OperationType::kWrite, exec_time);
      }
    }
  }

  std::vector<std::pair<OperationType, size_t>>
  GetResults()
  {
    return std::move(exec_times_nano_);
  }
};
