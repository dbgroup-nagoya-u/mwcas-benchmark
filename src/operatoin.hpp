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

#ifndef MWCAS_BENCHMARK_OPERATION_H
#define MWCAS_BENCHMARK_OPERATION_H

#include <array>

#include "common.hpp"

class Operation
{
 public:
  /*################################################################################################
   * Public constructors and assignment operators
   *##############################################################################################*/

  Operation() : targets_{} { targets_.fill(nullptr); }

  constexpr Operation(const Operation &) = default;
  constexpr Operation &operator=(const Operation &obj) = default;
  constexpr Operation(Operation &&) = default;
  constexpr Operation &operator=(Operation &&) = default;

  /*################################################################################################
   * Public destructors
   *##############################################################################################*/

  ~Operation() = default;

  /*################################################################################################
   * Public getters/setters
   *##############################################################################################*/

  constexpr uint64_t *
  GetAddr(const size_t i) const
  {
    return targets_.at(i);
  }

  void
  SetAddr(  //
      const size_t i,
      const uint64_t *addr)
  {
    targets_[i] = addr;
  }

 private:
  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  std::array<uint64_t *, kMaxTargetNum> targets_;
};

#endif  // MWCAS_BENCHMARK_OPERATION_H