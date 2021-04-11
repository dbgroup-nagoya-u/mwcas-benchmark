// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include <gflags/gflags.h>
#include <glog/logging.h>
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
DEFINE_int64(read_ratio, 0, "The ratio of MwCAS read operations");
DEFINE_int64(num_exec, 1000, "The number of executing MwCAS operations");
DEFINE_int64(num_thread, 1, "The number of executing threads");
DEFINE_int64(num_shared, 100, "The number of shared target fields of MwCAS");
DEFINE_int64(num_target, 2, "The number of private target fields of MwCAS");
DEFINE_bool(ours, true, "Use MwCAS library (dbgroup) as a benchmark target");
DEFINE_bool(microsoft, false, "Use PMwCAS library (Microsoft) as a benchmark target");
DEFINE_bool(single, false, "Use Single CAS as a benchmark target");

std::shared_mutex mtx;

class MwCASBench
{
 private:
  size_t read_ratio_;
  size_t num_exec_;
  size_t num_thread_;
  size_t num_shared_;
  size_t num_target_;
  size_t *shared_fields_;
  pmwcas::DescriptorPool *desc_pool_;

 public:
  MwCASBench()
  {
    // set input parameters
    read_ratio_ = FLAGS_read_ratio;
    num_exec_ = FLAGS_num_exec;
    num_thread_ = FLAGS_num_thread;
    num_shared_ = FLAGS_num_shared;
    num_target_ = FLAGS_num_target;
    pmwcas::InitLibrary(pmwcas::DefaultAllocator::Create, pmwcas::DefaultAllocator::Destroy,
                        pmwcas::LinuxEnvironment::Create, pmwcas::LinuxEnvironment::Destroy);
    desc_pool_ = new pmwcas::DescriptorPool{1024 * num_thread_, num_thread_};

    // create shared target fields
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

  Worker *
  CreateWorker(  //
      const BenchTarget target,
      const size_t random_seed)
  {
    switch (target) {
      case kOurs:
        return new WorkerMwCAS{shared_fields_, num_shared_, num_target_,
                               read_ratio_,    num_exec_,   random_seed};
      case kMicrosoft:
        return new WorkerPMwCAS{*desc_pool_, shared_fields_, num_shared_, num_target_,
                                read_ratio_, num_exec_,      random_seed};
      case kSingleCAS:
        return new WorkerSingleCAS{shared_fields_, num_shared_, num_target_,
                                   read_ratio_,    num_exec_,   random_seed};
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
    auto worker = CreateWorker(target, random_seed);

    {
      const auto lock = std::shared_lock<std::shared_mutex>(mtx);
    }
    worker->MeasureThroughput();

    p_result.set_value(worker);
  }

  void
  RunMwCASBench(const BenchTarget target)
  {
    std::vector<std::future<Worker *>> futures;
    {
      // create lock to prevent
      const auto lock = std::unique_lock<std::shared_mutex>(mtx);

      // create threads
      std::mt19937_64 rand_engine{0};
      for (size_t index = 0; index < num_thread_; ++index) {
        std::promise<Worker *> p_result;
        futures.emplace_back(p_result.get_future());
        std::thread{&MwCASBench::RunWorker, this, std::move(p_result), target, rand_engine()}
            .detach();
      }
    }

    std::cout << "Run workers." << std::endl;

    // gather results
    std::vector<Worker *> results;
    results.reserve(num_thread_);
    for (auto &&future : futures) {
      results.emplace_back(future.get());
    }

    std::cout << "Finish running." << std::endl;

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

    // std::cout << "MIN: " << sorted.front() << std::endl;
    // std::cout << "50%: " << sorted[num_exec_ * num_thread_ * 0.50] << std::endl;
    // std::cout << "90%: " << sorted[num_exec_ * num_thread_ * 0.90] << std::endl;
    // std::cout << "95%: " << sorted[num_exec_ * num_thread_ * 0.95] << std::endl;
    // std::cout << "99%: " << sorted[num_exec_ * num_thread_ * 0.99] << std::endl;
    // std::cout << "MAX: " << sorted.back() << std::endl;

    double throughput = (static_cast<double>(num_exec_) * num_thread_) / (avg_nano_time / 1E6);

    std::cout << "Throughput [Ops/s]: " << throughput << std::endl;
  }
};

int
main(int argc, char *argv[])
{
  // init glog with a program name
  google::InitGoogleLogging(argv[0]);

  // parse command line options
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  LOG(INFO) << "=== Start MwCAS Benchmark ===" << std::endl;
  auto bench = MwCASBench{};
  if (FLAGS_ours) {
    LOG(INFO) << "** Run our MwCAS..." << std::endl;
    bench.RunMwCASBench(BenchTarget::kOurs);
    LOG(INFO) << "** Finish." << std::endl;
  }
  if (FLAGS_microsoft) {
    LOG(INFO) << "** Run Microsoft's PMwCAS..." << std::endl;
    bench.RunMwCASBench(BenchTarget::kMicrosoft);
    LOG(INFO) << "** Finish." << std::endl;
  }
  if (FLAGS_single) {
    LOG(INFO) << "** Run Single CAS..." << std::endl;
    bench.RunMwCASBench(BenchTarget::kSingleCAS);
    LOG(INFO) << "** Finish." << std::endl;
  }
  LOG(INFO) << "==== End MwCAS Benchmark ====" << std::endl;

  return 0;
}
