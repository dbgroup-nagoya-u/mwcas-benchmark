// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

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
  size_t spinlock_time_ = 0;

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
