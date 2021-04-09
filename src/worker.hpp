// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <algorithm>
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

  bool seed_is_modified_;

  std::vector<Operation> operation_queue_;

  std::vector<size_t> exec_times_nano_;

  size_t exec_time_nano_;

  size_t shared_field_num_;

  size_t target_field_num_;

  std::vector<std::vector<size_t>> target_fields_;

 protected:
  /*################################################################################################
   * Inherited member variables
   *##############################################################################################*/

  size_t *shared_fields_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  Worker(  //
      size_t *shared_fields,
      const size_t shared_field_num,
      const size_t target_field_num,
      const size_t read_ratio,
      const size_t operation_counts,
      const size_t random_seed = 0)
      : read_ratio_{read_ratio},
        operation_counts_{operation_counts},
        random_seed_{random_seed},
        seed_is_modified_{true},
        exec_time_nano_{0},
        shared_field_num_{shared_field_num},
        target_field_num_{target_field_num},
        shared_fields_{shared_fields}
  {
    PrepareBench();
  }

  ~Worker() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  virtual void
  ReadMwCASField(const size_t index)
  {
  }

  virtual void
  PerformMwCAS(const std::vector<size_t> &target_fields)
  {
  }

  void
  PrepareBench()
  {
    // initialize execution time
    exec_time_nano_ = 0;
    exec_times_nano_.clear();
    exec_times_nano_.reserve(operation_counts_);

    // generate an operation-queue for benchmark
    if (seed_is_modified_) {
      seed_is_modified_ = false;

      operation_queue_.clear();
      operation_queue_.reserve(operation_counts_);
      target_fields_.clear();
      target_fields_.reserve(operation_counts_);

      std::mt19937_64 rand_engine{random_seed_};
      for (size_t i = 0; i < operation_counts_; ++i) {
        // create an operation-queue
        const auto ops = (rand_engine() % 100 < read_ratio_) ? Operation::kRead : Operation::kWrite;
        operation_queue_.emplace_back(ops);

        // select target fields for each operation
        std::vector<size_t> target_field;
        target_field.reserve(target_field_num_);
        for (size_t j = 0; j < target_field_num_; ++j) {
          target_field.emplace_back(rand_engine() % shared_field_num_);
          if (operation_queue_[i] == Operation::kRead) {
            break;
          }
        }
        std::sort(target_field.begin(), target_field.end());
        target_fields_.emplace_back(std::move(target_field));
      }
    }
  }

  void
  MeasureLatency()
  {
    assert(operation_queue_.size() == operation_counts_);
    assert(target_fields_.size() == operation_counts_);
    assert(exec_times_nano_.empty());

    for (size_t i = 0; i < operation_counts_; ++i) {
      const auto start_time = std::chrono::high_resolution_clock::now();
      switch (operation_queue_[i]) {
        case kRead:
          ReadMwCASField(target_fields_[i][0]);
          break;
        case kWrite:
          PerformMwCAS(target_fields_[i]);
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
    assert(target_fields_.size() == operation_counts_);

    const auto start_time = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < operation_counts_; ++i) {
      switch (operation_queue_[i]) {
        case kRead:
          ReadMwCASField(target_fields_[i][0]);
          break;
        case kWrite:
          PerformMwCAS(target_fields_[i]);
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
