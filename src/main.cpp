// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include <future>
#include <thread>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "mwcas/mwcas_manager.hpp"
#include "worker.hpp"
#include "worker_mwcas.hpp"

// set command line options
DEFINE_int64(read_ratio, 0, "The ratio of MwCAS read operations");
DEFINE_int64(num_exec, 10000, "The number of executing MwCAS operations");
DEFINE_int64(num_thread, 8, "The number of executing threads");
DEFINE_int64(num_shared, 2, "The number of shared target fields of MwCAS");
DEFINE_int64(num_private, 0, "The number of private target fields of MwCAS");
DEFINE_bool(test, false, "test");

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
  RunMwCASBench()
  {
    // create shared target fields
    size_t shared_fields[kFieldNum];
    for (size_t index = 0; index < kFieldNum; ++index) {
      shared_fields[index] = 0;
    }

    // create MwCAS manager
    auto manager = dbgroup::atomic::mwcas::MwCASManager{1000};

    // lambda function to run benchmark in multi-threads
    auto f = [&](std::promise<std::vector<std::pair<OperationType, size_t>>> p) {
      auto worker =
          WorkerMwCAS{&manager, num_shared_, num_private_, shared_fields, read_ratio_, num_exec_};
      worker.Run();
      p.set_value(worker.GetResults());
    };

    // run benchmark and get results
    std::vector<std::future<std::vector<std::pair<OperationType, size_t>>>> futures;
    for (size_t index = 0; index < num_thread_; ++index) {
      std::promise<std::vector<std::pair<OperationType, size_t>>> p;
      futures.emplace_back(p.get_future());
      auto t = std::thread{f, std::move(p)};
      t.detach();
    }
    std::vector<std::vector<std::pair<OperationType, size_t>>> results;
    for (size_t index = 0; index < num_thread_; ++index) {
      results.emplace_back(futures[index].get());
    }

    LOG(INFO) << "finish MwCAS benchmark.";

    size_t avg = 0;
    for (auto &&vec : results) {
      for (auto &&[op, exec_time] : vec) {
        avg += exec_time;
      }
      avg /= vec.size();
    }

    LOG(INFO) << "avg_latency: " << avg;
  }
};

int
main(int argc, char *argv[])
{
  // init glog with a program name
  google::InitGoogleLogging(argv[0]);

  // parse command line options
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  LOG(INFO) << "=== Start MwCAS Benchmark ===";
  auto bench = MwCASBench{};
  bench.RunMwCASBench();
  LOG(INFO) << "==== End MwCAS Benchmark ====";

  return 0;
}
