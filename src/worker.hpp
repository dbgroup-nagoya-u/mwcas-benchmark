// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <array>
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

  size_t random_seed_;

  std::vector<Operation> operation_queue_;

  std::vector<size_t> exec_times_nano_;

  size_t exec_time_nano_;

 protected:
  /*################################################################################################
   * Inherited member variables
   *##############################################################################################*/

  size_t shared_num_;

  size_t private_num_;

  size_t *shared_fields_;

  size_t private_fields_[kFieldNum];

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  Worker(  //
      const size_t shared_field_num,
      const size_t private_field_num,
      size_t *shared_fields,
      const size_t read_ratio,
      const size_t operation_counts,
      const size_t random_seed = 0)
      : read_ratio_{read_ratio},
        operation_counts_{operation_counts},
        random_seed_{random_seed},
        exec_time_nano_{0},
        shared_num_{shared_field_num},
        private_num_{private_field_num}
  {
    PrepareBench();
    shared_fields_ = shared_fields;
    for (size_t count = 0; count < private_field_num; ++count) {
      private_fields_[count] = 0;
    }
  }

  ~Worker() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  virtual void
  ReadMwCASField()
  {
  }

  virtual void
  PerformMwCAS()
  {
  }

  void
  PrepareBench()
  {
    // initialize vectors
    exec_times_nano_.clear();
    exec_times_nano_.reserve(operation_counts_);
    operation_queue_.clear();
    operation_queue_.reserve(operation_counts_);

    // generate an operation-queue for benchmark
    std::mt19937_64 rand_engine{random_seed_};
    for (size_t i = 0; i < operation_counts_; ++i) {
      const auto ops = (rand_engine() % 100 < read_ratio_) ? Operation::kRead : Operation::kWrite;
      operation_queue_.emplace_back(ops);
    }
  }

  void
  MeasureLatency()
  {
    assert(operation_queue_.size() == operation_counts_);
    assert(exec_times_nano_.empty());

    for (size_t i = 0; i < operation_counts_; ++i) {
      const auto start_time = std::chrono::high_resolution_clock::now();
      switch (operation_queue_[i]) {
        case kRead:
          ReadMwCASField();
          break;
        case kWrite:
          PerformMwCAS();
          break;
        default:
          break;
      }
      const auto end_time = std::chrono::high_resolution_clock::now();
      const auto exec_time =
          std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
      exec_times_nano_.emplace_back(exec_time);
    }
  }

  void
  MeasureThroughput()
  {
    assert(operation_queue_.size() == operation_counts_);

    const auto start_time = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < operation_counts_; ++i) {
      switch (operation_queue_[i]) {
        case kRead:
          ReadMwCASField();
          break;
        case kWrite:
          PerformMwCAS();
          break;
        default:
          break;
      }
    }
    const auto end_time = std::chrono::high_resolution_clock::now();
    exec_time_nano_ =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
  }

  std::pair<const std::vector<Operation> &, const std::vector<size_t> &>
  GetLatencies() const
  {
    return {operation_queue_, exec_times_nano_};
  }

  size_t
  GetTotalExecTime() const
  {
    return exec_time_nano_;
  }
};
