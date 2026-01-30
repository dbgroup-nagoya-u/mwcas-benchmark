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

// C++ standard libraries
#include <cstddef>
#include <iostream>
#include <random>
#include <string>

// external system libraries
#include <gflags/gflags.h>

// external libraries
#include "dbgroup/benchmark/benchmarker.hpp"
#include "dbgroup/benchmark/validator.hpp"

// local sources
#include "dbgroup/mwcas_benchmark/mwcas_target.hpp"
#include "dbgroup/mwcas_benchmark/operation_engine.hpp"

/*##############################################################################
 * Options for selecting competitors
 *############################################################################*/

DEFINE_bool(  //
    dlf_mwcas,
    false,
    "Use deadlock-free MwCAS as a competitor.");

DEFINE_bool(  //
    casn,
    false,
    "Use CASN algorithm as a competitor.");

DEFINE_bool(  //
    aopt,
    false,
    "Use AOPT algorithm as a competitor.");

DEFINE_bool(  //
    lf_mwcas,
    false,
    "Use lock-free MwCAS as a competitor.");

DEFINE_bool(  //
    pmwcas,
    false,
#ifdef MWCAS_BENCH_USE_PMWCAS
    "Use microsoft/pmwcas as a competitor."
#else
    "microsoft/pmwcas is disabled. Turn on 'MWCAS_BENCH_USE_PMWCAS'."
#endif
);

/*##############################################################################
 * Options for controling workload
 *############################################################################*/

DEFINE_uint64(  //
    num_thread,
    8,
    "The number of worker threads for benchmarking.");

DEFINE_double(  //
    skew_parameter,
    1,
    "A skew parameter (based on Zipf's law).");

DEFINE_uint64(  //
    arr_cap,
    1000000,
    "The capacity of an array for MwCAS targets.");

/*##############################################################################
 * Utility options
 *############################################################################*/

DEFINE_string(  //
    seed,
    "",
    "A random seed for reproducibility.");

DEFINE_uint64(  //
    timeout,
    10,
    "Timeout in seconds.");

DEFINE_bool(  //
    csv,
    false,
    "Output benchmark results as a CSV format.");

DEFINE_bool(  //
    throughput,
    true,
    "true: measure throughput, false: measure latency.");

/*##############################################################################
 * Option validators
 *############################################################################*/

DEFINE_validator(num_thread, &::dbgroup::benchmark::ValidateThreadNum);
DEFINE_validator(skew_parameter, &::dbgroup::benchmark::ValidateSkewParameter);
DEFINE_validator(arr_cap, &::dbgroup::benchmark::ValidatePositiveValue);
DEFINE_validator(seed, &::dbgroup::benchmark::ValidateStr2UInt);
DEFINE_validator(timeout, &::dbgroup::benchmark::ValidatePositiveValue);

/*##############################################################################
 * Utility functions
 *############################################################################*/

template <class Impl>
void
RunBenchmark(  //
    const std::string &target_name,
    const size_t target_num)
{
  using OperationEngine = ::dbgroup::OperationEngine;
  using Target = ::dbgroup::MwCASTarget<Impl>;
  using Benchmarker = ::dbgroup::benchmark::Benchmarker<Target, OperationEngine>;
  using Builder = typename Benchmarker::Builder;

  const auto seed = (FLAGS_seed.empty()) ? std::random_device{}() : std::stoul(FLAGS_seed);
  Target target{FLAGS_arr_cap, FLAGS_num_thread};
  OperationEngine ops_engine{target_num, FLAGS_arr_cap, FLAGS_skew_parameter, seed};

  Builder builder{target, target_name, ops_engine};
  builder.SetThreadNum(FLAGS_num_thread);
  builder.SetTimeOut(FLAGS_timeout);
  builder.SetRandomSeed(seed);
  if (FLAGS_csv) {
    builder.OutputAsCSV(FLAGS_throughput);
  }
  auto &&bench = builder.Build();
  bench->Run();
}

/*##############################################################################
 * Main function
 *############################################################################*/

auto
main(  //
    int argc,
    char *argv[])  //
    -> int
{
  // parse command line options
  constexpr bool kRemoveParsedFlags = true;
  gflags::SetUsageMessage("measures throughput/latency of MwCAS implementations.");
  gflags::ParseCommandLineFlags(&argc, &argv, kRemoveParsedFlags);

  // parse command line arguments
  if (argc < 2) {
    std::cerr << "Usage: ./pmwcas_bench --<competitor> <target_word_num>\n";
    return 1;
  }
  const auto target_num = std::stoull(argv[1]);  // NOLINT
  constexpr auto kMax = ::dbgroup::atomic::mwcas::kMwCASCapacity;
  if (target_num > kMax) {
    std::cerr << "[Error] The current benchmark can swap up to " << kMax << " words.\n";
    return 1;
  }

  // run benchmark for each implementaton
  if (FLAGS_dlf_mwcas) RunBenchmark<DLFMwCAS>("Deadlock-free MwCAS", target_num);
  if (FLAGS_casn) RunBenchmark<CASN>("CASN", target_num);
  if (FLAGS_aopt) RunBenchmark<AOPT>("AOPT", target_num);
  if (FLAGS_lf_mwcas) {
    RunBenchmark<LFMwCAS>("Lock-free MwCAS", target_num);
    std::cout << "MaxWrapCounts: "
              << dbgroup::atomic::mwcas::lock_free::MwCASDescriptor::CalcMaxVersionWrapCountSum()
              << std::endl;
    auto sw = dbgroup::atomic::mwcas::lock_free::MwCASDescriptor::GetStopWatch();
    std::cout << "StopWatch 0.00p: " << sw.Quantile(0.00) << std::endl;
    std::cout << "StopWatch 0.25p: " << sw.Quantile(0.25) << std::endl;
    std::cout << "StopWatch 0.50p: " << sw.Quantile(0.50) << std::endl;
    std::cout << "StopWatch 0.75p: " << sw.Quantile(0.75) << std::endl;
    std::cout << "StopWatch 0.90p: " << sw.Quantile(0.90) << std::endl;
    std::cout << "StopWatch 0.95p: " << sw.Quantile(0.95) << std::endl;
    std::cout << "StopWatch 0.99p: " << sw.Quantile(0.99) << std::endl;
    std::cout << "StopWatch 0.999p: " << sw.Quantile(0.999) << std::endl;
    std::cout << "StopWatch 0.9999p: " << sw.Quantile(0.9999) << std::endl;
    std::cout << "StopWatch 1.00p: " << sw.Quantile(1.00) << std::endl;
  }
#ifdef MWCAS_BENCH_USE_PMWCAS
  if (FLAGS_pmwcas) RunBenchmark<PMwCAS>("PMwCAS", target_num);
#endif

  return 0;
}
