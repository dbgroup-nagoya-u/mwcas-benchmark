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
  using MwCASTargets = ::std::array<uint64_t *, kMaxTargetNum>;

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
      std::vector<uint64_t *> &target_fields,
      const size_t target_num,
      const size_t operation_counts,
      ZipfGenerator &zipf_engine,
      const size_t random_seed = 0)
      : total_exec_time_nano_{0}, target_num_{target_num}
  {
    // reserve capacity of dynamic arrays
    latencies_nano_.reserve(operation_counts);
    operations_.reserve(operation_counts);

    // generate an operation-queue for benchmarking
    std::mt19937_64 rand_engine{random_seed};
    for (size_t i = 0; i < operation_counts; ++i) {
      // select target addresses for i-th operation
      MwCASTargets targets;
      for (size_t j = 0; j < target_num; ++j) {
        const auto cur_end = targets.begin() + j;
        uint64_t *addr;
        do {
          addr = target_fields[zipf_engine(rand_engine)];
        } while (std::find(targets.begin(), cur_end, addr) != cur_end);
        targets[j] = addr;
      }

      // sort target addresses for linearization
      std::sort(targets.begin(), targets.end());

      operations_.emplace_back(std::move(targets));
    }
  }

  /*################################################################################################
   * Public destructors
   *##############################################################################################*/

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
    assert(latencies_nano_.empty());

    for (auto &&operation : operations_) {
      const auto start_time = std::chrono::high_resolution_clock::now();

      PerformMwCAS(operation);

      const auto end_time = std::chrono::high_resolution_clock::now();
      const auto exec_time =
          std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
      latencies_nano_.emplace_back(exec_time);
    }
  }

  /**
   * @brief Measure and store total execution time.
   *
   */
  void
  MeasureThroughput()
  {
    const auto start_time = std::chrono::high_resolution_clock::now();

    for (auto &&operation : operations_) {
      PerformMwCAS(operation);
    }

    const auto end_time = std::chrono::high_resolution_clock::now();
    total_exec_time_nano_ =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
  }

  /**
   * @brief Sort execution time to compute percentiled latency.
   *
   */
  void
  SortExecutionTimes()
  {
    std::sort(latencies_nano_.begin(), latencies_nano_.end());
  }

  /**
   * @param index a target index to get latency
   * @return size_t `index`-th execution time
   */
  size_t
  GetLatency(const size_t index) const
  {
    return latencies_nano_[index];
  }

  /**
   * @return size_t total execution time
   */
  size_t
  GetTotalExecTime() const
  {
    return total_exec_time_nano_;
  }

  /**
   * @return size_t the number of executed MwCAS operations
   */
  size_t
  GetOperationCount() const
  {
    return operations_.size();
  }

 private:
  /*################################################################################################
   * Internal utility functions
   *##############################################################################################*/

  /**
   * @brief Perform a MwCAS operation based on its implementation.
   *
   * @param targets target addresses of a MwCAS operation
   */
  void PerformMwCAS(const MwCASTargets &targets);

  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  /// total execution time [ns]
  size_t total_exec_time_nano_;

  /// execution time for each operation [ns]
  std::vector<size_t> latencies_nano_;

  /// MwCAS target filed addresses for each operation
  std::vector<MwCASTargets> operations_;

  /// the number of MwCAS targets for each operation
  const size_t target_num_;
};

/*##################################################################################################
 * Specializations for each MwCAS implementations
 *################################################################################################*/

template <>
inline void
Worker<MwCAS>::PerformMwCAS(const MwCASTargets &targets)
{
  while (true) {
    MwCAS desc{};
    for (size_t i = 0; i < target_num_; ++i) {
      const auto addr = targets[i];
      const auto old_val = dbgroup::atomic::mwcas::ReadMwCASField<size_t>(addr);
      const auto new_val = old_val + 1;
      desc.AddMwCASTarget(addr, old_val, new_val);
    }

    if (desc.MwCAS()) break;
  }
}

template <>
inline void
Worker<PMwCAS>::PerformMwCAS(const MwCASTargets &targets)
{
  using PMwCASField = ::pmwcas::MwcTargetField<uint64_t>;

  while (true) {
    auto desc = pmwcas_desc_pool->AllocateDescriptor();
    auto epoch = pmwcas_desc_pool->GetEpoch();
    epoch->Protect();
    for (size_t i = 0; i < target_num_; ++i) {
      const auto addr = targets[i];
      const auto old_val = reinterpret_cast<PMwCASField *>(addr)->GetValueProtected();
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
Worker<SingleCAS>::PerformMwCAS(const MwCASTargets &targets)
{
  for (size_t i = 0; i < target_num_; ++i) {
    auto target = reinterpret_cast<SingleCAS *>(targets[i]);
    auto old_val = target->load(std::memory_order_relaxed);
    size_t new_val;
    do {
      new_val = old_val + 1;
    } while (!target->compare_exchange_weak(old_val, new_val, std::memory_order_relaxed));
  }
}
