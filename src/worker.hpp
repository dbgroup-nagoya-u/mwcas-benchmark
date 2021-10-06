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

#include <algorithm>
#include <array>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include "common.hpp"
#include "random/zipf.hpp"

inline std::unique_ptr<PMwCAS> pmwcas_desc_pool = nullptr;

/**
 * @brief A class for a worker thread for benchmarking.
 *
 * @tparam Wrapper
 */
template <class Wrapper>
class Worker
{
  /*################################################################################################
   * Type aliases
   *##############################################################################################*/

  using ZipfGenerator = ::dbgroup::random::zipf::ZipfGenerator;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  /**
   * @brief Construct a new Worker object.
   *
   * @param operation_counts the number of operations executed in each thread
   * @param random_seed a random seed
   */
  Worker(  //
      size_t *target_fields,
      const size_t mwcas_target_num,
      const size_t operation_counts,
      ZipfGenerator &zipf_engine,
      const size_t random_seed = 0)
      : operation_counts_{operation_counts},
        random_seed_{random_seed},
        exec_time_nano_{0},
        zipf_engine_{zipf_engine},
        target_fields_{target_fields},
        mwcas_target_num_{mwcas_target_num}
  {
    exec_times_nano_.reserve(operation_counts_);

    // generate an operation-queue for benchmark
    mwcas_targets_.reserve(operation_counts_);

    std::mt19937_64 rand_engine{random_seed_};
    for (size_t i = 0; i < operation_counts_; ++i) {
      // select target fields for i-th operation
      std::array<size_t, kMaxTargetNum> mwcas_target;
      for (size_t j = 0; j < mwcas_target_num_; ++j) {
        const auto current_end = mwcas_target.begin() + j;
        size_t field_id;
        do {
          field_id = zipf_engine_(rand_engine);
        } while (std::find(mwcas_target.begin(), current_end, field_id) != current_end);

        mwcas_target[j] = field_id;
      }

      // sort MwCAS targets to prevent deadlocks
      std::sort(mwcas_target.begin(), mwcas_target.begin() + mwcas_target_num_);
      mwcas_targets_.emplace_back(std::move(mwcas_target));
    }
  }

  ~Worker() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  /**
   * @brief Measure and store execution time for each operation.
   *
   */
  void
  MeasureLatency()
  {
    assert(mwcas_targets_.size() == operation_counts_);
    assert(exec_times_nano_.empty());

    for (size_t i = 0; i < operation_counts_; ++i) {
      const auto start_time = std::chrono::high_resolution_clock::now();

      PerformMwCAS(mwcas_targets_[i]);

      const auto end_time = std::chrono::high_resolution_clock::now();
      const auto exec_time =
          std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
      exec_times_nano_.emplace_back(exec_time);
    }
  }

  /**
   * @brief Measure and store total execution time.
   *
   */
  void
  MeasureThroughput()
  {
    assert(mwcas_targets_.size() == operation_counts_);

    const auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < operation_counts_; ++i) {
      PerformMwCAS(mwcas_targets_[i]);
    }

    const auto end_time = std::chrono::high_resolution_clock::now();
    exec_time_nano_ =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
  }

  /**
   * @brief Sort execution time to compute percentiled latency.
   *
   */
  void
  SortExecutionTimes()
  {
    std::sort(exec_times_nano_.begin(), exec_times_nano_.end());
  }

  /**
   * @param index a target index to get latency
   * @return size_t `index`-th execution time
   */
  size_t
  GetLatency(const size_t index) const
  {
    return exec_times_nano_[index];
  }

  /**
   * @return size_t total execution time
   */
  size_t
  GetTotalExecTime() const
  {
    return exec_time_nano_;
  }

  /**
   * @return size_t the number of executed MwCAS operations
   */
  size_t
  GetOperationCount() const
  {
    return operation_counts_;
  }

 private:
  /*################################################################################################
   * Internal utility functions
   *##############################################################################################*/

  /**
   * @brief Perform a MwCAS operation based on its implementation.
   *
   * @param target_indexes target indexes of a MwCAS operation
   */
  void PerformMwCAS(const std::array<size_t, kMaxTargetNum> &target_indexes);

  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  /// the number of operations executed in each thread
  size_t operation_counts_;

  /// a random seed
  size_t random_seed_;

  /// total execution time [ns]
  size_t exec_time_nano_;

  /// execution time for each operation [ns]
  std::vector<size_t> exec_times_nano_;

  /// MwCAS target filed IDs for each operation
  std::vector<std::array<size_t, kMaxTargetNum>> mwcas_targets_;

  /// a random engine according to Zipf's law
  ZipfGenerator &zipf_engine_;

  /// a head of MwCAS target fields
  size_t *target_fields_;

  /// the number of MwCAS targets for each operation
  size_t mwcas_target_num_;
};

/*##################################################################################################
 * Specializations for each MwCAS implementations
 *################################################################################################*/

template <>
inline void
Worker<MwCAS>::PerformMwCAS(const std::array<size_t, kMaxTargetNum> &target_indexes)
{
  while (true) {
    MwCAS desc{};
    for (size_t i = 0; i < mwcas_target_num_; ++i) {
      const auto addr = target_fields_ + target_indexes[i];
      const auto old_val = dbgroup::atomic::mwcas::ReadMwCASField<size_t>(addr);
      const auto new_val = old_val + 1;
      desc.AddMwCASTarget(addr, old_val, new_val);
    }
    if (desc.MwCAS()) break;
  }
}

template <>
inline void
Worker<PMwCAS>::PerformMwCAS(const std::array<size_t, kMaxTargetNum> &target_indexes)
{
  while (true) {
    auto desc = pmwcas_desc_pool->AllocateDescriptor();
    auto epoch = pmwcas_desc_pool->GetEpoch();
    epoch->Protect();
    for (size_t i = 0; i < mwcas_target_num_; ++i) {
      const auto addr = target_fields_ + target_indexes[i];
      const auto old_val =
          reinterpret_cast<pmwcas::MwcTargetField<size_t> *>(addr)->GetValueProtected();
      const auto new_val = old_val + 1;
      desc->AddEntry(addr, old_val, new_val);
    }
    auto success = desc->MwCAS();
    epoch->Unprotect();
    if (success) break;
  }
}

template <>
inline void
Worker<SingleCAS>::PerformMwCAS(const std::array<size_t, kMaxTargetNum> &target_indexes)
{
  for (size_t i = 0; i < mwcas_target_num_; ++i) {
    const auto addr = target_fields_ + target_indexes[i];
    auto target = reinterpret_cast<std::atomic_size_t *>(addr);
    auto old_val = target->load(std::memory_order_relaxed);
    size_t new_val;
    do {
      new_val = old_val + 1;
    } while (!target->compare_exchange_weak(old_val, new_val, std::memory_order_relaxed));
  }
}
