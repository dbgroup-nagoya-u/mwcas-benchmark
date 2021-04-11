// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include <gflags/gflags.h>
#include <pmwcas.h>

#include <future>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <thread>

#include "worker.hpp"
#include "worker_cas.hpp"
#include "worker_mwcas.hpp"
#include "worker_pmwcas.hpp"

// set command line options
DEFINE_uint64(read_ratio, 0, "The ratio of MwCAS read operations");
DEFINE_uint64(num_exec, 10000, "The number of MwCAS operations executed in each thread");
DEFINE_uint64(num_loop, 1, "The number of loops to measure performance");
DEFINE_uint64(num_thread, 1, "The number of execution threads");
DEFINE_uint64(num_shared, 10000, "The number of total target fields");
DEFINE_uint64(num_target, 2, "The number of target fields for each MwCAS");
DEFINE_bool(ours, true, "Use MwCAS library (DB Group @ Nagoya Univ.) as a benchmark target");
DEFINE_bool(microsoft, false, "Use PMwCAS library (Microsoft) as a benchmark target");
DEFINE_bool(single, false, "Use Single CAS as a benchmark target");
DEFINE_bool(csv, false, "Output benchmark results as CSV format");

/*##################################################################################################
 * Global variables
 *################################################################################################*/

/// a mutex to trigger workers simultaneously
std::shared_mutex trigger_worker;

/// a mutex to wait until all workers have been created
std::shared_mutex worker_created;

/// a flag to control output format
bool output_format_is_text = true;

/*##################################################################################################
 * Global utility functions
 *################################################################################################*/

void
Log(const char *message)
{
  if (output_format_is_text) {
    std::cout << message << std::endl;
  }
}

void
LogThroughput(const double throughput)
{
  if (output_format_is_text) {
    std::cout << "Throughput [MOps/s]: " << throughput << std::endl;
  } else {
    std::cout << throughput << std::endl;
  }
}

/*##################################################################################################
 * Utility Classes
 *################################################################################################*/

/**
 * @brief A class to run MwCAS benchmark.
 *
 */
class MwCASBench
{
 private:
  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  /// a ratio of read operations
  const size_t read_ratio_;

  /// the number of MwCAS operations executed in each thread
  const size_t num_exec_;

  /// the number of loops to measure performance
  const size_t num_loop_;

  /// the number of execution threads
  const size_t num_thread_;

  /// the number of total target fields
  const size_t num_shared_;

  /// the number of target fields for each MwCAS
  const size_t num_target_;

  /// target fields of MwCAS
  size_t *shared_fields_;

  /// PMwCAS descriptor pool
  pmwcas::DescriptorPool *desc_pool_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  MwCASBench()
      : read_ratio_{FLAGS_read_ratio},
        num_exec_{FLAGS_num_exec},
        num_loop_{FLAGS_num_loop},
        num_thread_{FLAGS_num_thread},
        num_shared_{FLAGS_num_shared},
        num_target_{FLAGS_num_target}
  {
    // prepare PMwCAS descriptor pool
    pmwcas::InitLibrary(pmwcas::DefaultAllocator::Create, pmwcas::DefaultAllocator::Destroy,
                        pmwcas::LinuxEnvironment::Create, pmwcas::LinuxEnvironment::Destroy);
    desc_pool_ = new pmwcas::DescriptorPool{static_cast<uint32_t>(4096 * num_thread_),
                                            static_cast<uint32_t>(num_thread_)};

    // prepare shared target fields
    shared_fields_ = new size_t[num_shared_];
    for (size_t index = 0; index < num_shared_; ++index) {
      shared_fields_[index] = 0;
    }
  }

  ~MwCASBench()
  {
    delete shared_fields_;
    delete desc_pool_;
  }

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  Worker *
  CreateWorker(  //
      const BenchTarget target,
      const size_t random_seed)
  {
    switch (target) {
      case kOurs:
        return new WorkerMwCAS{shared_fields_, num_shared_, num_target_, read_ratio_,
                               num_exec_,      num_loop_,   random_seed};
      case kMicrosoft:
        return new WorkerPMwCAS{*desc_pool_, shared_fields_, num_shared_, num_target_,
                                read_ratio_, num_exec_,      num_loop_,   random_seed};
      case kSingleCAS:
        return new WorkerSingleCAS{shared_fields_, num_shared_, num_target_, read_ratio_,
                                   num_exec_,      num_loop_,   random_seed};
      default:
        return nullptr;
    }
  }

  void
  RunWorker(  //
      std::promise<Worker *> p_result,
      const BenchTarget target,
      const size_t random_seed)
  {
    // prepare a worker
    Worker *worker;
    {
      const auto lock = std::shared_lock<std::shared_mutex>(worker_created);
      worker = CreateWorker(target, random_seed);
      // unlock to notice worker has been created
    }

    {
      // wait for all workers to be created
      const auto lock = std::shared_lock<std::shared_mutex>(trigger_worker);
    }
    worker->MeasureThroughput();

    p_result.set_value(worker);
  }

  void
  RunMwCASBench(const BenchTarget target)
  {
    std::vector<std::future<Worker *>> futures;
    {
      // create lock to prevent workers from running
      const auto trigger_lock = std::unique_lock<std::shared_mutex>(trigger_worker);

      // create threads
      std::mt19937_64 rand_engine{0};
      for (size_t index = 0; index < num_thread_; ++index) {
        std::promise<Worker *> p_result;
        futures.emplace_back(p_result.get_future());
        std::thread{&MwCASBench::RunWorker, this, std::move(p_result), target, rand_engine()}
            .detach();
      }

      // wait for all workers to be created
      const auto worker_lock = std::unique_lock<std::shared_mutex>(worker_created);
    }

    Log("Run workers...");

    // gather results
    std::vector<Worker *> results;
    results.reserve(num_thread_);
    for (auto &&future : futures) {
      results.emplace_back(future.get());
    }

    Log("Finish running...");

    size_t avg_nano_time = 0;
    for (auto &&worker : results) {
      avg_nano_time += worker->GetTotalExecTime();
      delete worker;
    }
    avg_nano_time /= num_thread_;

    // std::vector<size_t> sorted;
    // for (auto &&vec : results) {
    //   for (auto &&[op, exec_time] : vec) {
    //     sorted.emplace_back(exec_time);
    //   }
    // }
    // std::sort(sorted.begin(), sorted.end());

    // Log("MIN: " << sorted.front());
    // Log("50%: " << sorted[num_exec_ * num_thread_ * 0.50]);
    // Log("90%: " << sorted[num_exec_ * num_thread_ * 0.90]);
    // Log("95%: " << sorted[num_exec_ * num_thread_ * 0.95]);
    // Log("99%: " << sorted[num_exec_ * num_thread_ * 0.99]);
    // Log("MAX: " << sorted.back());

    double throughput =
        static_cast<double>(num_exec_ * num_loop_ * num_thread_) / (avg_nano_time / 1E3);

    LogThroughput(throughput);
  }
};

/*##################################################################################################
 * Main function
 *################################################################################################*/

int
main(int argc, char *argv[])
{
  // parse command line options
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  output_format_is_text = !FLAGS_csv;

  Log("=== Start MwCAS Benchmark ===");
  auto bench = MwCASBench{};
  if (FLAGS_ours) {
    Log("** Run our MwCAS...");
    bench.RunMwCASBench(BenchTarget::kOurs);
    Log("** Finish.");
  }
  if (FLAGS_microsoft) {
    Log("** Run Microsoft's PMwCAS...");
    bench.RunMwCASBench(BenchTarget::kMicrosoft);
    Log("** Finish.");
  }
  if (FLAGS_single) {
    Log("** Run Single CAS...");
    bench.RunMwCASBench(BenchTarget::kSingleCAS);
    Log("** Finish.");
  }
  Log("==== End MwCAS Benchmark ====");

  return 0;
}
