// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <algorithm>
#include <array>
#include <random>
#include <utility>
#include <vector>

#include "common.hpp"
#include "random/zipf.hpp"

using ::dbgroup::random::zipf::ZipfGenerator;

class Worker
{
 private:
  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  size_t read_ratio_;

  size_t operation_counts_;

  size_t loop_num_;

  size_t random_seed_;

  std::vector<Operation> operation_queue_;

  std::vector<std::array<size_t, kMaxTargetNum>> mwcas_targets_;

  size_t exec_time_nano_;

  std::vector<size_t> exec_times_nano_;

  size_t target_filed_num_;

  double skew_parameter_;

  /*################################################################################################
   * Internal utility functions
   *##############################################################################################*/

  void
  PrepareBench()
  {
    // initialize execution time
    exec_time_nano_ = 0;
    exec_times_nano_.reserve(operation_counts_ * loop_num_);

    // generate an operation-queue for benchmark
    operation_queue_.reserve(operation_counts_);
    mwcas_targets_.reserve(operation_counts_);

    std::mt19937_64 rand_engine{random_seed_};
    ZipfGenerator zipf_engine{target_filed_num_, skew_parameter_, random_seed_};
    for (size_t i = 0; i < operation_counts_; ++i) {
      // select i-th operation
      const auto ops = (rand_engine() % 100 < read_ratio_) ? Operation::kRead : Operation::kWrite;
      operation_queue_.emplace_back(ops);

      // select target fields for i-th operation
      std::array<size_t, kMaxTargetNum> mwcas_target;
      for (size_t j = 0; j < mwcas_target_num_; ++j) {
        auto field_id = zipf_engine();
        const auto current_end = mwcas_target.begin() + j;
        while (std::find(mwcas_target.begin(), current_end, field_id) != current_end) {
          // if there is a selected id in targets, pick next to it
          ++field_id;
        }

        mwcas_target[j] = field_id;
        if (operation_queue_[i] == Operation::kRead) {
          // a read operation requires only one target
          break;
        }
      }

      // sort MwCAS targets to prevent deadlocks
      std::sort(mwcas_target.begin(), mwcas_target.begin() + mwcas_target_num_);
      mwcas_targets_.emplace_back(std::move(mwcas_target));
    }
  }

 protected:
  /*################################################################################################
   * Inherited member variables
   *##############################################################################################*/

  size_t *target_fields_;

  size_t mwcas_target_num_;

  /*################################################################################################
   * Inherited utility functions
   *##############################################################################################*/

  virtual void ReadMwCASField(const size_t index) = 0;

  virtual void PerformMwCAS(const std::array<size_t, kMaxTargetNum> &target_fields) = 0;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  Worker(  //
      size_t *target_fields,
      const size_t target_field_num,
      const size_t mwcas_target_num,
      const size_t read_ratio,
      const size_t operation_counts,
      const size_t loop_num,
      const double skew_parameter,
      const size_t random_seed = 0)
      : read_ratio_{read_ratio},
        operation_counts_{operation_counts},
        loop_num_{loop_num},
        random_seed_{random_seed},
        exec_time_nano_{0},
        target_filed_num_{target_field_num},
        skew_parameter_{skew_parameter},
        target_fields_{target_fields},
        mwcas_target_num_{mwcas_target_num}
  {
    PrepareBench();
  }

  virtual ~Worker() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  void
  MeasureLatency()
  {
    assert(operation_queue_.size() == operation_counts_);
    assert(mwcas_targets_.size() == operation_counts_);
    assert(exec_times_nano_.empty());

    for (size_t loop = 0; loop < loop_num_; ++loop) {
      for (size_t i = 0; i < operation_counts_; ++i) {
        const auto start_time = std::chrono::high_resolution_clock::now();
        switch (operation_queue_[i]) {
          case kRead:
            ReadMwCASField(mwcas_targets_[i][0]);
            break;
          case kWrite:
            PerformMwCAS(mwcas_targets_[i]);
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
  }

  void
  MeasureThroughput()
  {
    assert(operation_queue_.size() == operation_counts_);
    assert(mwcas_targets_.size() == operation_counts_);

    const auto start_time = std::chrono::high_resolution_clock::now();
    for (size_t loop = 0; loop < loop_num_; ++loop) {
      for (size_t i = 0; i < operation_counts_; ++i) {
        switch (operation_queue_[i]) {
          case kRead:
            ReadMwCASField(mwcas_targets_[i][0]);
            break;
          case kWrite:
            PerformMwCAS(mwcas_targets_[i]);
            break;
          default:
            break;
        }
      }
    }

    const auto end_time = std::chrono::high_resolution_clock::now();
    exec_time_nano_ =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
  }

  void
  SortExecutionTimes()
  {
    std::sort(exec_times_nano_.begin(), exec_times_nano_.end());
  }

  size_t
  GetLatency(const size_t index) const
  {
    return exec_times_nano_[index];
  }

  size_t
  GetTotalExecTime() const
  {
    return exec_time_nano_;
  }
};
