// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include "worker_cas.hpp"

#include "gtest/gtest.h"

class WorkerSingleCASFixture : public ::testing::Test
{
 public:
  static constexpr size_t kTargetFieldNum = 1;
  static constexpr size_t kTargetNum = 1;
  static constexpr size_t kOperationNum = 1000;
  static constexpr double kSkewParameter = 0;
  static constexpr size_t kRandomSeed = 0;

  std::unique_ptr<size_t[]> target_fields;
  ZipfGenerator zipf_engine_;
  std::unique_ptr<WorkerSingleCAS> worker;

 protected:
  void
  SetUp() override
  {
    target_fields = std::make_unique<size_t[]>(kTargetFieldNum);
    for (size_t i = 0; i < kTargetFieldNum; ++i) {
      target_fields[i] = 0;
    }

    zipf_engine_ = ZipfGenerator{kTargetFieldNum, kSkewParameter};

    worker = std::make_unique<WorkerSingleCAS>(target_fields.get(), kTargetNum, kOperationNum,
                                               zipf_engine_, kRandomSeed);
  }

  void
  TearDown() override
  {
  }
};

TEST_F(WorkerSingleCASFixture, MeasureThroughput_SwapSameFields_ReadCorrectValues)
{
  worker->MeasureThroughput();

  for (size_t i = 0; i < kTargetFieldNum; ++i) {
    EXPECT_EQ(target_fields[i], kOperationNum);
  }
}

TEST_F(WorkerSingleCASFixture, MeasureThroughput_SwapSameFields_MeasureReasonableExecutionTime)
{
  const auto start_time = std::chrono::high_resolution_clock::now();
  worker->MeasureThroughput();
  const auto end_time = std::chrono::high_resolution_clock::now();
  const auto total_time =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

  EXPECT_GE(worker->GetTotalExecTime(), 0);
  EXPECT_LE(worker->GetTotalExecTime(), total_time);
}

TEST_F(WorkerSingleCASFixture, MeasureLatency_SwapSameFields_ReadCorrectValues)
{
  worker->MeasureLatency();

  for (size_t i = 0; i < kTargetFieldNum; ++i) {
    EXPECT_EQ(target_fields[i], kOperationNum);
  }
}

TEST_F(WorkerSingleCASFixture, MeasureLatency_SwapSameFields_MeasureReasonableLatency)
{
  const auto start_time = std::chrono::high_resolution_clock::now();
  worker->MeasureLatency();
  const auto end_time = std::chrono::high_resolution_clock::now();
  const auto total_time =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

  worker->SortExecutionTimes();

  EXPECT_GE(worker->GetLatency(0), 0);
  for (size_t i = 1; i < kOperationNum; ++i) {
    EXPECT_GE(worker->GetLatency(i), worker->GetLatency(i - 1));
  }
  EXPECT_LE(worker->GetLatency(kOperationNum - 1), total_time);
}
