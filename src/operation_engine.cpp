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
#include <random>

namespace dbgroup
{

OperationEngine::OperationEngine(  //
    const size_t target_num,
    const size_t array_cap,
    const double skew_param,
    const size_t random_seed)
    : arr_cap_{array_cap}, target_num_{target_num}, skew_parameter_{skew_param}
{
  pos_index_.reserve(array_cap);
  for (size_t i = 0; i < array_cap; ++i) {
    pos_index_.emplace_back(i);
  }
  std::mt19937_64 rand_engine{random_seed};
  std::shuffle(pos_index_.begin(), pos_index_.end(), rand_engine);
}

auto
OperationEngine::OPIter::operator++()  //
    -> OPIter &
{
  // generate unique targets
  auto &&cur_end = positions_.begin();
  for (size_t i = 0; i < target_num_; ++i, ++cur_end) {
    auto pos = zipf_(rand_);
    while (std::find(positions_.begin(), cur_end, pos) != cur_end) {
      // continue until the different target is selected
      ++pos;
    }
    positions_[i] = pos;
  }

  // convert to array positions and sort
  for (size_t i = 0; i < target_num_; ++i) {
    positions_[i] = pos_index_[positions_[i]];
  }
  std::sort(positions_.begin(), positions_.end());

  return *this;
}

}  // namespace dbgroup
