// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include "worker_mwcas.hpp"

#include "gtest/gtest.h"

class WorkerMwCASFixture : public ::testing::Test
{
 public:
  static constexpr size_t kTargetFieldNum = 2;
  static constexpr size_t kTargetNum = 2;
  static constexpr size_t kReadRatio = 0;
  static constexpr size_t kOperationNum = 1000;
  static constexpr size_t kLoopNum = 1;
  static constexpr double kSkewParameter = 0;
  static constexpr size_t kRandomSeed = 0;

  std::unique_ptr<size_t[]> target_fields;

 protected:
  void
  SetUp() override
  {
    target_fields = std::make_unique<size_t[]>(kTargetFieldNum);
    for (size_t i = 0; i < kTargetFieldNum; ++i) {
      target_fields[i] = 0;
    }
  }

  void
  TearDown() override
  {
  }
};

TEST_F(WorkerMwCASFixture, MeasureThroughput_SwapSameFields_ReadCorrectValues)
{
  WorkerMwCAS worker{target_fields.get(), kTargetFieldNum, kTargetNum,     kReadRatio,
                     kOperationNum,       kLoopNum,        kSkewParameter, kRandomSeed};

  worker.MeasureThroughput();

  for (size_t i = 0; i < kTargetFieldNum; ++i) {
    EXPECT_EQ(target_fields[i], kOperationNum);
  }
}

TEST_F(WorkerMwCASFixture, MeasureLatency_SwapSameFields_ReadCorrectValues)
{
  WorkerMwCAS worker{target_fields.get(), kTargetFieldNum, kTargetNum,     kReadRatio,
                     kOperationNum,       kLoopNum,        kSkewParameter, kRandomSeed};

  worker.MeasureLatency();

  for (size_t i = 0; i < kTargetFieldNum; ++i) {
    EXPECT_EQ(target_fields[i], kOperationNum);
  }
}
