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
#include <random>
#include <utility>
#include <vector>

#include "common.hpp"

/**
 * @brief An abstract class for a worker thread for benchmarking.
 *
 */
class Worker
{
 protected:
  /*################################################################################################
   * Inherited member variables
   *##############################################################################################*/

  /// the number of operations executed in each thread
  size_t operation_counts_;

  /// a random seed
  size_t random_seed_;

  /// total execution time [ns]
  size_t exec_time_nano_;

  /// execution time for each operation [ns]
  std::vector<size_t> exec_times_nano_;

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
      const size_t operation_counts,
      const size_t random_seed = 0)
      : operation_counts_{operation_counts}, random_seed_{random_seed}, exec_time_nano_{0}
  {
    exec_times_nano_.reserve(operation_counts_);
  }

  virtual ~Worker() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  /**
   * @brief Measure and store execution time for each operation.
   *
   */
  virtual void MeasureLatency() = 0;

  /**
   * @brief Measure and store total execution time.
   *
   */
  virtual void MeasureThroughput() = 0;

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
};
