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
