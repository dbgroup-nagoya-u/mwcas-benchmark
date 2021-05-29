// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <cstdint>

/**
 * @brief A list of MwCAS operations.
 *
 */
enum Operation
{
  kFront,
  kBack,
  kPush,
  kPop
};

/**
 * @brief A list of MwCAS implementations.
 *
 */
enum BenchTarget
{
  kOurs,
  kPMwCAS,
  kSingleCAS,
  kQueueCAS,
  kQueueMwCAS,
  kQueueMutex
};

#ifdef MWCAS_BENCH_MAX_FIELD_NUM
/// the maximum number of MwCAS targets
constexpr size_t kMaxTargetNum = MWCAS_BENCH_MAX_FIELD_NUM;
#else
constexpr size_t kMaxTargetNum = 8;
#endif
