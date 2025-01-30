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

// the corresponding header
#include "dbgroup/mwcas_benchmark/operation_engine.hpp"

// C++ standard libraries
#include <cstddef>
#include <cstdint>
#include <vector>

// external libraries
#include "gtest/gtest.h"

namespace dbgroup::benchmark::test
{
class OperationEngineFixture : public ::testing::Test
{
 protected:
  /*############################################################################
   * Constants
   *##########################################################################*/

  static constexpr size_t kTargetNum = 8;
  static constexpr size_t kArrayCapacity = 1E6;
  static constexpr double kSkew = 1.0;
  static constexpr size_t kSeed = 0;
  static constexpr size_t kLoopNum = 1E6;

  /*############################################################################
   * Setup/Teardown
   *##########################################################################*/

  void
  SetUp() override
  {
  }

  void
  TearDown() override
  {
  }
};

/*------------------------------------------------------------------------------
 * Test definitions
 *----------------------------------------------------------------------------*/

TEST_F(OperationEngineFixture, OPIterGenerateUniqueAndRandomTargets)
{
  const OperationEngine ops_engine{kTargetNum, kArrayCapacity, kSkew, kSeed};
  auto &&iter = ops_engine.GetOPIter(0, kSeed);

  std::vector<size_t> prev_positions(kTargetNum, 0);
  for (size_t i = 0; i < kLoopNum; ++i, ++iter) {
    const auto &positions = (*iter).second;
    EXPECT_EQ(positions.size(), prev_positions.size());

    int64_t prev_pos = -1;
    for (const auto &pos : positions) {
      const auto next_pos = static_cast<int64_t>(pos);
      EXPECT_GT(next_pos, prev_pos);
      prev_pos = next_pos;
    }

    bool has_diff = false;
    for (size_t j = 0; j < kTargetNum; ++j) {
      has_diff |= positions.at(j) != prev_positions.at(j);
    }
    EXPECT_TRUE(has_diff);

    prev_positions = positions;
  }
}

}  // namespace dbgroup::benchmark::test
