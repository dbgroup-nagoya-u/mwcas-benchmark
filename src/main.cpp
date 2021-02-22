// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include <future>
#include <thread>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "mwcas/mwcas_manager.hpp"
#include "worker.hpp"
#include "worker_mwcas.hpp"
#include "worker_pmwcas.hpp"

// set command line options
DEFINE_int64(read_ratio, 0, "The ratio of MwCAS read operations");
DEFINE_int64(num_exec, 10000, "The number of executing MwCAS operations");
DEFINE_int64(num_thread, 1, "The number of executing threads");
DEFINE_int64(num_shared, 1, "The number of shared target fields of MwCAS");
DEFINE_int64(num_private, 0, "The number of private target fields of MwCAS");
DEFINE_bool(ours, true, "Use MwCAS library (dbgroup) as a benchmark target");
DEFINE_bool(microsoft, false, "Use PMwCAS library (Microsoft) as a benchmark target");

class MwCASBench
{
 private:
  size_t read_ratio_;
  size_t num_exec_;
  size_t num_thread_;
  size_t num_shared_;
  size_t num_private_;

 public:
  MwCASBench()
  {
    // set input parameters
    read_ratio_ = FLAGS_read_ratio;
    num_exec_ = FLAGS_num_exec;
    num_thread_ = FLAGS_num_thread;
    num_shared_ = FLAGS_num_shared;
    num_private_ = FLAGS_num_private;

    if (num_shared_ + num_private_ == 0) {
      LOG(FATAL) << "MwCAS must have at least one target field.";
    }
  }

  ~MwCASBench() = default;

  void
  RunOurMwCAS(  //
      std::promise<std::vector<std::pair<OperationType, size_t>>> p,
      dbgroup::atomic::mwcas::MwCASManager *manager,
      size_t *shared_fields)
  {
    auto worker =
        WorkerMwCAS{manager, num_shared_, num_private_, shared_fields, read_ratio_, num_exec_};
    worker.Run();
    p.set_value(worker.GetResults());
  }

  void
  RunMicrosoftPMwCAS(  //
      std::promise<std::vector<std::pair<OperationType, size_t>>> p,
      pmwcas::DescriptorPool *desc_pool,
      size_t *shared_fields)
  {
    auto worker =
        WorkerPMwCAS{desc_pool, num_shared_, num_private_, shared_fields, read_ratio_, num_exec_};
    worker.Run();
    p.set_value(worker.GetResults());
  }

  void
  RunMwCASBench(const BenchTarget target)
  {
    // create shared target fields
    size_t shared_fields[kFieldNum];
    for (size_t index = 0; index < kFieldNum; ++index) {
      shared_fields[index] = 0;
    }

    // create MwCAS managers
    auto manager = dbgroup::atomic::mwcas::MwCASManager{1000};
    auto desc_pool = pmwcas::DescriptorPool{1024, 8};

    // run benchmark and get results
    std::vector<std::future<std::vector<std::pair<OperationType, size_t>>>> futures;
    for (size_t index = 0; index < num_thread_; ++index) {
      std::promise<std::vector<std::pair<OperationType, size_t>>> p;
      futures.emplace_back(p.get_future());
      std::thread t;
      switch (target) {
        case kOurs:
          t = std::thread{&MwCASBench::RunOurMwCAS, this, std::move(p), &manager, shared_fields};
          break;
        case kMicrosoft:
          t = std::thread{&MwCASBench::RunMicrosoftPMwCAS, this, std::move(p), &desc_pool,
                          shared_fields};
          break;
      }
      t.detach();
    }
    std::vector<std::vector<std::pair<OperationType, size_t>>> results;
    for (size_t index = 0; index < num_thread_; ++index) {
      results.emplace_back(futures[index].get());
    }

    size_t avg = 0;
    for (auto &&vec : results) {
      for (auto &&[op, exec_time] : vec) {
        avg += exec_time;
      }
      avg /= vec.size();
    }

    LOG(INFO) << "Average latency: " << avg << std::endl;
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
  LOG(INFO) << "==== End MwCAS Benchmark ====" << std::endl;

  return 0;
}
