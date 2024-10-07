/*
 * Copyright 2024 Database Group, Nagoya University
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

// the corresponding header
#include "dbgroup/mwcas_benchmark/operation_engine.hpp"

// C++ standard libraries
#include <algorithm>
#include <cstddef>

namespace dbgroup
{

OperationEngine::OperationEngine(  //
    const size_t target_num,
    const size_t array_cap,
    const double skew_param,
    const size_t random_seed)
    : target_num_{target_num}, zipf_dist_{0, array_cap - 1, skew_param}
{
  pos_index_.reserve(array_cap);
  for (size_t i = 0; i < array_cap; ++i) {
    pos_index_.emplace_back(i);
  }
  std::mt19937_64 rand_engine{random_seed};
  std::shuffle(pos_index_.begin(), pos_index_.end(), rand_engine);
}

auto
OperationEngine::Generate(  //
    const size_t n,
    const size_t random_seed) const  //
    -> std::vector<Operation>
{
  std::mt19937_64 rand_engine{random_seed};

  // generate an operation-queue for benchmarking
  std::vector<Operation> operations{};
  operations.reserve(n);
  for (size_t i = 0; i < n; ++i) {
    // select target addresses for i-th operation
    Operation ops{};
    for (size_t j = 0; j < target_num_; ++j) {
      auto pos = zipf_dist_(rand_engine);
      while (!ops.SetPositionIfUnique(pos_index_.at(pos))) {
        // continue until the different target is selected
        pos = zipf_dist_(rand_engine);
      }
    }
    ops.SortTargets();
    operations.emplace_back(ops);
  }

  return operations;
}

}  // namespace dbgroup
