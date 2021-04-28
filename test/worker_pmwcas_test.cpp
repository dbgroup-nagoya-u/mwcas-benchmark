// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include "worker_pmwcas.hpp"

#include <pmwcas.h>

#include "gtest/gtest.h"

class WorkerPMwCASFixture : public ::testing::Test
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
  std::unique_ptr<WorkerPMwCAS> worker;
  std::unique_ptr<pmwcas::DescriptorPool> desc_pool;

 protected:
  void
  SetUp() override
  {
    pmwcas::InitLibrary(pmwcas::DefaultAllocator::Create, pmwcas::DefaultAllocator::Destroy,
                        pmwcas::LinuxEnvironment::Create, pmwcas::LinuxEnvironment::Destroy);
    desc_pool = std::make_unique<pmwcas::DescriptorPool>(1024, 8);

    target_fields = std::make_unique<size_t[]>(kTargetFieldNum);
    for (size_t i = 0; i < kTargetFieldNum; ++i) {
      target_fields[i] = 0;
    }

    worker = std::make_unique<WorkerPMwCAS>(*desc_pool.get(), target_fields.get(), kTargetFieldNum,
                                            kTargetNum, kReadRatio, kOperationNum, kLoopNum,
                                            kSkewParameter, kRandomSeed);
  }

  void
  TearDown() override
  {
  }
};

TEST_F(WorkerPMwCASFixture, MeasureThroughput_SwapSameFields_ReadCorrectValues)
{
  worker->MeasureThroughput();

  for (size_t i = 0; i < kTargetFieldNum; ++i) {
    EXPECT_EQ(target_fields[i], kOperationNum);
  }
}

TEST_F(WorkerPMwCASFixture, MeasureThroughput_SwapSameFields_MeasureReasonableExecutionTime)
{
  const auto start_time = std::chrono::high_resolution_clock::now();
  worker->MeasureThroughput();
  const auto end_time = std::chrono::high_resolution_clock::now();
  const auto total_time =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

  EXPECT_GT(worker->GetTotalExecTime(), 0);
  EXPECT_LT(worker->GetTotalExecTime(), total_time);
}

TEST_F(WorkerPMwCASFixture, MeasureLatency_SwapSameFields_ReadCorrectValues)
{
  worker->MeasureLatency();

  for (size_t i = 0; i < kTargetFieldNum; ++i) {
    EXPECT_EQ(target_fields[i], kOperationNum);
  }
}

TEST_F(WorkerPMwCASFixture, MeasureLatency_SwapSameFields_MeasureReasonableLatency)
{
  const auto start_time = std::chrono::high_resolution_clock::now();
  worker->MeasureLatency();
  const auto end_time = std::chrono::high_resolution_clock::now();
  const auto total_time =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

  worker->SortExecutionTimes();

  EXPECT_GT(worker->GetLatency(0), 0);
  for (size_t i = 1; i < kOperationNum; ++i) {
    EXPECT_GE(worker->GetLatency(i), worker->GetLatency(i - 1));
  }
  EXPECT_LT(worker->GetLatency(kOperationNum - 1), total_time);
}
