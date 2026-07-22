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

#ifndef DBGROUP_MWCAS_BENCHMARK_MWCAS_TARGET_HPP_
#define DBGROUP_MWCAS_BENCHMARK_MWCAS_TARGET_HPP_

// C++ standard libraries
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

// external system libraries
#include <gflags/gflags.h>

// external C++ libraries
#include <dbgroup/constants.hpp>
#include <dbgroup/memory/utility.hpp>

// local sources
#include "dbgroup/mwcas_benchmark/operation_engine.hpp"

/*############################################################################*
 * Competitors
 *############################################################################*/

#include <dbgroup/atomic/mwcas/deadlock_free/mwcas_descriptor.hpp>
using DLFMwCAS = ::dbgroup::atomic::mwcas::deadlock_free::MwCASDescriptor;

#include <dbgroup/atomic/mwcas/lock_free/casn_descriptor.hpp>
using CASN = ::dbgroup::atomic::mwcas::lock_free::CASNDescriptor;

#include <dbgroup/atomic/mwcas/lock_free/aopt_descriptor.hpp>
using AOPT = ::dbgroup::atomic::mwcas::lock_free::AOPTDescriptor;

#include <dbgroup/atomic/mwcas/lock_free/mwcas_descriptor.hpp>
using LFMwCAS = ::dbgroup::atomic::mwcas::lock_free::MwCASDescriptor;

#include <dbgroup/atomic/mwcas/lock_free/mwcas_descriptor_weak.hpp>
using LFMwCASWeak = ::dbgroup::atomic::mwcas::lock_free::MwCASDescriptorWeak;

#ifdef MWCAS_BENCH_USE_PMWCAS
// C++ standard libraries
#include <memory>

// competitor's headers
#include <mwcas/mwcas.h>
#include <pmwcas.h>
using PMwCAS = ::pmwcas::DescriptorPool;
#endif

/*############################################################################*
 * Class declarations
 *############################################################################*/

namespace dbgroup
{
/**
 * @brief A class to deal with MwCAS target data and algorithms.
 *
 * @tparam Impl A certain implementation of MwCAS algorithms.
 */
template <class Impl>
class MwCASTarget
{
 public:
  /*##########################################################################*
   * Public types
   *##########################################################################*/

  using OPType = OperationEngine::OPType;
  using Operation = OperationEngine::Operation;

  /*##########################################################################*
   * Public constructors and assignment operators
   *##########################################################################*/

  explicit MwCASTarget(  //
      const size_t array_cap,
      [[maybe_unused]] const size_t thread_num)
      : target_fields_{array_cap, CacheLineBlock{0}}
  {
#ifdef MWCAS_BENCH_USE_PMWCAS
    // prepare descriptor pool for PMwCAS if needed
    if constexpr (std::is_same_v<Impl, PMwCAS>) {
      constexpr uint32_t kPartition = thread_num;
      constexpr uint32_t kPoolCapacity = kPartition * 1024;
      ::pmwcas::InitLibrary(  //
          pmwcas::DefaultAllocator::Create, pmwcas::DefaultAllocator::Destroy,
          pmwcas::LinuxEnvironment::Create, pmwcas::LinuxEnvironment::Destroy);
      pmwcas_desc_pool_ = std::make_unique<PMwCAS>(kPoolCapacity, kPartition);
    }
#endif

    if constexpr (std::is_same_v<Impl, CASN>            //
                  || std::is_same_v<Impl, AOPT>         //
                  || std::is_same_v<Impl, LFMwCASWeak>  //
                  || std::is_same_v<Impl, LFMwCAS>) {
      const auto cleaner_num = 1UL + static_cast<size_t>(thread_num / 24);  // NOLINT
      Impl::StartGC(::dbgroup::memory::kDefaultGCTime, cleaner_num);
    }
  }

  MwCASTarget(const MwCASTarget&) = delete;
  MwCASTarget(MwCASTarget&&) = delete;

  MwCASTarget& operator=(const MwCASTarget& obj) = delete;
  MwCASTarget& operator=(MwCASTarget&&) = delete;

  /*##########################################################################*
   * Public destructors
   *##########################################################################*/

  ~MwCASTarget()
  {
    if constexpr (std::is_same_v<Impl, CASN>            //
                  || std::is_same_v<Impl, AOPT>         //
                  || std::is_same_v<Impl, LFMwCASWeak>  //
                  || std::is_same_v<Impl, LFMwCAS>) {
      Impl::StopGC();
    }
  }

  /*##########################################################################*
   * Public APIs
   *##########################################################################*/

  constexpr void
  SetUpForWorker() const
  {
    // do nothing
  }

  constexpr void
  PreProcess() const
  {
    // do nothing
  }

  constexpr void
  PostProcess() const
  {
    // do nothing
  }

  constexpr void
  TearDownForWorker() const
  {
    // do nothing
  }

  /**
   * @brief Perform a PMwCAS operation.
   *
   * @param type A dummy input.
   * @param positions MwCAS target positions.
   * @return The number of executed operations (i.e., 1).
   */
  auto Execute(  //
      OPType type,
      const Operation& positions)  //
      -> size_t;

 private:
  /*##########################################################################*
   * Internal classes
   *##########################################################################*/

  /**
   * @brief A class for aligning memory block into cache lines.
   *
   */
  struct alignas(kCacheLineSize) CacheLineBlock {
    uint64_t val;
  };

  /*##########################################################################*
   * Internal member variables
   *##########################################################################*/

  /// @brief Target fields for MwCAS operations.
  std::vector<CacheLineBlock> target_fields_{};

#ifdef MWCAS_BENCH_USE_PMWCAS
  /// @brief The pool of descriptors for microsoft/pmwcas.
  std::unique_ptr<PMwCAS> pmwcas_desc_pool_{};
#endif
};

}  // namespace dbgroup

#endif  // DBGROUP_MWCAS_BENCHMARK_MWCAS_TARGET_HPP_
