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

#include <gflags/gflags.h>
#include <pmwcas.h>

#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "worker.hpp"

/*##################################################################################################
 * Global variables
 *################################################################################################*/

/// a mutex to control workers
std::shared_mutex mutex_1st;

/// a mutex to control workers
std::shared_mutex mutex_2nd;

/// a flag to control output format
bool output_format_is_text = true;

/*##################################################################################################
 * Global utility functions
 *################################################################################################*/

/**
 * @brief Log a message to stdout if the output mode is `text`.
 *
 * @param message an output message
 */
void
Log(const char *message)
{
  if (output_format_is_text) {
    std::cout << message << std::endl;
  }
}

/*##################################################################################################
 * Utility Classes
 *################################################################################################*/

/**
 * @brief A class to run MwCAS benchmark.
 *
 */
template <class MwCASImplementation>
class MwCASBench
{
  /*################################################################################################
   * Type aliases
   *##############################################################################################*/

  using Worker_t = Worker<MwCASImplementation>;
  using ZipfGenerator = ::dbgroup::random::zipf::ZipfGenerator;

 public:
  /*################################################################################################
   * Public constructors and assignment operators
   *##############################################################################################*/

  MwCASBench(  //
      const size_t num_exec,
      const size_t num_thread,
      const size_t num_field,
      const size_t num_target,
      const double skew_parameter,
      const size_t random_seed,
      const bool measure_throughput)
      : exec_num_{num_exec},
        thread_num_{num_thread},
        target_field_num_{num_field},
        target_num_{num_target},
        skew_parameter_{skew_parameter},
        random_seed_{random_seed},
        measure_throughput_{measure_throughput},
        zipf_engine_{num_field, skew_parameter}
  {
    // prepare shared target fields
    target_fields_ = std::make_unique<size_t[]>(target_field_num_);
    InitializeTargetFields();

    if constexpr (std::is_same_v<MwCASImplementation, PMwCAS>) {
      // prepare PMwCAS descriptor pool
      pmwcas::InitLibrary(pmwcas::DefaultAllocator::Create, pmwcas::DefaultAllocator::Destroy,
                          pmwcas::LinuxEnvironment::Create, pmwcas::LinuxEnvironment::Destroy);
      pmwcas_desc_pool = std::make_unique<PMwCAS>(static_cast<uint32_t>(8192 * thread_num_),
                                                  static_cast<uint32_t>(thread_num_));
    }
  }

  /*################################################################################################
   * Public destructors
   *##############################################################################################*/

  ~MwCASBench() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  /**
   * @brief Run MwCAS benchmark and output results to stdout.
   *
   * @param target a target implementation
   */
  void
  Run()
  {
    /*----------------------------------------------------------------------------------------------
     * Preparation of benchmark workers
     *--------------------------------------------------------------------------------------------*/
    std::vector<std::future<Worker_t *>> futures;

    {  // create a lock to stop workers from running
      const auto lock = std::unique_lock<std::shared_mutex>(mutex_1st);

      // create workers in each thread
      std::mt19937_64 rand_engine{random_seed_};
      size_t sum_exec_num = 0;
      for (size_t i = 0; i < thread_num_; ++i) {
        std::promise<Worker_t *> p;
        futures.emplace_back(p.get_future());

        const size_t exec_num =
            (i < thread_num_ - 1) ? exec_num_ / thread_num_ : exec_num_ - sum_exec_num;
        std::thread t{&MwCASBench::RunWorker, this, std::move(p), exec_num, rand_engine()};
        t.detach();

        sum_exec_num += exec_num;
      }
      assert(sum_exec_num == exec_num_);

      // wait for all workers to be created
      const auto guard = std::unique_lock<std::shared_mutex>(mutex_2nd);

      InitializeTargetFields();
    }  // unlock to run workers

    /*----------------------------------------------------------------------------------------------
     * Measuring throughput/latency
     *--------------------------------------------------------------------------------------------*/
    if (measure_throughput_) {
      Log("Run workers to measure throughput...");
    } else {
      Log("Run workers to measure latency...");
    }

    {  // create a lock to stop workers from running
      const auto lock = std::unique_lock<std::shared_mutex>(mutex_2nd);

      // wait for all workers to finish measuring throughput
      const auto guard = std::unique_lock<std::shared_mutex>(mutex_1st);
    }  // unlock to run workers

    /*----------------------------------------------------------------------------------------------
     * Output benchmark results
     *--------------------------------------------------------------------------------------------*/
    Log("Finish running...");

    std::vector<Worker_t *> results;
    results.reserve(thread_num_);
    for (auto &&future : futures) {
      results.emplace_back(future.get());
    }

    if (measure_throughput_) {
      LogThroughput(results);
    } else {
      LogLatency(results);
    }

    for (auto &&worker : results) {
      delete worker;
    }
  }

 private:
  /*################################################################################################
   * Internal utility functions
   *##############################################################################################*/

  /**
   * @brief Compute a throughput score and output it to stdout.
   *
   * @param workers worker pointers that hold benchmark results
   */
  void
  LogThroughput(const std::vector<Worker_t *> &workers) const
  {
    size_t avg_nano_time = 0;
    for (auto &&worker : workers) {
      avg_nano_time += worker->GetTotalExecTime();
    }
    avg_nano_time /= thread_num_;

    const auto throughput = exec_num_ / (avg_nano_time / 1E9);

    if (output_format_is_text) {
      std::cout << "Throughput [Ops/s]: " << throughput << std::endl;
    } else {
      std::cout << throughput;
    }
  }

  /**
   * @brief Compute percentiled latency and output it to stdout.
   *
   * @param workers worker pointers that hold benchmark results
   */
  void
  LogLatency(const std::vector<Worker_t *> &workers) const
  {
    size_t lat_0 = std::numeric_limits<size_t>::max(), lat_90, lat_95, lat_99, lat_100;

    // compute the minimum latency and initialize an index list
    std::vector<size_t> indexes;
    indexes.reserve(thread_num_);
    for (size_t thread = 0; thread < thread_num_; ++thread) {
      indexes.emplace_back(workers[thread]->GetOperationCount() - 1);
      const auto exec_time = workers[thread]->GetLatency(0);
      if (exec_time < lat_0) {
        lat_0 = exec_time;
      }
    }

    // check latency with descending order
    for (size_t count = exec_num_; count >= exec_num_ * 0.90; --count) {
      size_t target_thread = 0;
      auto max_exec_time = std::numeric_limits<size_t>::min();
      for (size_t thread = 0; thread < thread_num_; ++thread) {
        const auto exec_time = workers[thread]->GetLatency(indexes[thread]);
        if (exec_time > max_exec_time) {
          max_exec_time = exec_time;
          target_thread = thread;
        }
      }

      // if `count` reaches target percentiles, store its latency
      if (count == exec_num_) {
        lat_100 = max_exec_time;
      } else if (count == static_cast<size_t>(exec_num_ * 0.99)) {
        lat_99 = max_exec_time;
      } else if (count == static_cast<size_t>(exec_num_ * 0.95)) {
        lat_95 = max_exec_time;
      } else if (count == static_cast<size_t>(exec_num_ * 0.90)) {
        lat_90 = max_exec_time;
      }

      --indexes[target_thread];
    }

    Log("Percentiled Latencies [ns]:");
    if (output_format_is_text) {
      std::cout << "  MIN: " << lat_0 << std::endl;
      std::cout << "  90%: " << lat_90 << std::endl;
      std::cout << "  95%: " << lat_95 << std::endl;
      std::cout << "  99%: " << lat_99 << std::endl;
      std::cout << "  MAX: " << lat_100 << std::endl;
    } else {
      std::cout << lat_0 << "," << lat_90 << "," << lat_95 << "," << lat_99 << "," << lat_100;
    }
  }

  /**
   * @brief Fill MwCAS target fields with zeros.
   *
   */
  void
  InitializeTargetFields()
  {
    assert(target_fields_);

    for (size_t index = 0; index < target_field_num_; ++index) {
      target_fields_[index] = 0;
    }
  }

  /**
   * @brief Run a worker thread to measure throuput and latency.
   *
   * @param p a promise of a worker pointer that holds benchmark results
   * @param target a target implementation
   * @param random_seed a random seed
   */
  void
  RunWorker(  //
      std::promise<Worker_t *> p,
      const size_t exec_num,
      const size_t random_seed)
  {
    // prepare a worker
    Worker_t *worker;

    {  // create a lock to stop a main thread
      const auto lock = std::shared_lock<std::shared_mutex>(mutex_2nd);
      worker = new Worker_t{target_fields_.get(), target_num_, exec_num, zipf_engine_, random_seed};
    }  // unlock to notice that this worker has been created

    {  // wait for benchmark to be ready
      const auto guard = std::shared_lock<std::shared_mutex>(mutex_1st);
      if (measure_throughput_) {
        worker->MeasureThroughput();
      } else {
        worker->MeasureLatency();
      }
    }  // unlock to notice that this worker has measured thuroughput/latency

    {  // wait for all workers to finish
      const auto guard = std::shared_lock<std::shared_mutex>(mutex_2nd);
      worker->SortExecutionTimes();
    }

    p.set_value(worker);
  }

  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  /// the total number of MwCAS operations for benchmarking
  const size_t exec_num_;

  /// the number of execution threads
  const size_t thread_num_;

  /// the total number of target fields
  const size_t target_field_num_;

  /// the number of MwCAS targets for each operation
  const size_t target_num_;

  /// a skew parameter
  const double skew_parameter_;

  /// a base random seed
  const size_t random_seed_;

  /// a flag to measure throughput (if true) or latency (if false)
  const bool measure_throughput_;

  /// target fields of MwCAS
  std::unique_ptr<size_t[]> target_fields_;

  /// a random engine according to Zipf's law
  ZipfGenerator zipf_engine_;
};
