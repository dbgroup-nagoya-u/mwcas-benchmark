// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <algorithm>
#include <array>
#include <random>
#include <utility>
#include <vector>

#include "common.hpp"
#include "worker.hpp"

/**
 * @brief An abstract class for a worker thread for benchmarking.
 *
 */
template <class Queue>
class WorkerQueue : public Worker
{
 private:
  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  /// target operations for benchmarking
  std::vector<Operation> operations_;

  /// a thread-safe queue
  Queue* queue_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  /**
   * @brief Construct a new WorkerQueue object.
   *
   * @param queue a target thread-safe queue
   * @param operation_counts the number of operations executed in each thread
   * @param random_seed a random seed
   */
  WorkerQueue(  //
      Queue* queue,
      const size_t operation_counts,
      const size_t random_seed = 0)
      : Worker{operation_counts, random_seed}, queue_{queue}
  {
    // generate an operation-queue for benchmark
    operations_.reserve(operation_counts_);

    std::mt19937_64 rand_engine{random_seed_};
    for (size_t i = 0; i < operation_counts_; ++i) {
      // select target fields for i-th operation
      Operation ops;
      const auto rand_val = rand_engine() % 100;
      if (rand_val < 25) {
        ops = Operation::kFront;
      } else if (rand_val < 50) {
        ops = Operation::kBack;
      } else if (rand_val < 75) {
        ops = Operation::kPush;
      } else {
        ops = Operation::kPop;
      }

      operations_.emplace_back(ops);
    }
  }

  virtual ~WorkerQueue() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  /**
   * @brief Measure and store execution time for each operation.
   *
   */
  void
  MeasureLatency() override
  {
    assert(operations_.size() == operation_counts_);
    assert(exec_times_nano_.empty());

    for (size_t i = 0; i < operation_counts_; ++i) {
      const auto start_time = std::chrono::high_resolution_clock::now();

      switch (operations_[i]) {
        case kFront:
          queue_->front();
          break;
        case kBack:
          queue_->back();
          break;
        case kPush:
          queue_->push(i);
          break;
        case kPop:
          queue_->pop();
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

  /**
   * @brief Measure and store total execution time.
   *
   */
  void
  MeasureThroughput() override
  {
    assert(operations_.size() == operation_counts_);

    const auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < operation_counts_; ++i) {
      switch (operations_[i]) {
        case kFront:
          queue_->front();
          break;
        case kBack:
          queue_->back();
          break;
        case kPush:
          queue_->push(i);
          break;
        case kPop:
          queue_->pop();
          break;
        default:
          break;
      }
    }

    const auto end_time = std::chrono::high_resolution_clock::now();
    exec_time_nano_ =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
  }
};
