// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <memory>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

class QueueFixture : public ::testing::Test
{
 public:
  static constexpr size_t kRepeatNum = 1E5;
  static constexpr size_t kThreadNum = 8;

  std::unique_ptr<Queue_t> queue_ = nullptr;

  void
  PushElements(const size_t repeat_num)
  {
    for (size_t i = 0; i < repeat_num; ++i) {
      queue_->push(i);
    }
  }

  void
  PopElements(const size_t repeat_num)
  {
    for (size_t i = 0; i < repeat_num; ++i) {
      queue_->pop();
    }
  }

 protected:
  void SetUp();

  void TearDown();
};

TEST_F(QueueFixture, Construct_Default_QueueCorrectlyInitialized)
{
  EXPECT_TRUE(queue_->empty());
  EXPECT_TRUE(queue_->IsValid());
}

TEST_F(QueueFixture, PushFront_OneItem_QueueIsNotEmpty)
{
  queue_->push(0);
  EXPECT_FALSE(queue_->empty());
  EXPECT_TRUE(queue_->IsValid());
}

TEST_F(QueueFixture, PushFront_ByMultiThreads_AllItemsPushed)
{
  std::vector<std::thread> threads;
  for (size_t i = 0; i < kThreadNum; ++i) {
    threads.emplace_back(&QueueFixture::PushElements, this, kRepeatNum);
  }
  for (auto &&thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(queue_->IsValid());

  for (size_t i = 0; i < kRepeatNum * kThreadNum; ++i) {
    EXPECT_FALSE(queue_->empty());
    queue_->pop();
  }
  EXPECT_TRUE(queue_->empty());
}

TEST_F(QueueFixture, PopBack_AfterPushFront_QueueIsEmpty)
{
  queue_->push(0);
  queue_->pop();
  EXPECT_TRUE(queue_->empty());
  EXPECT_TRUE(queue_->IsValid());
}

TEST_F(QueueFixture, PopBack_ByMultiThreads_AllItemsPopped)
{
  PushElements(kRepeatNum * kThreadNum);

  std::vector<std::thread> threads;
  for (size_t i = 0; i < kThreadNum; ++i) {
    threads.emplace_back(&QueueFixture::PopElements, this, kRepeatNum);
  }
  for (auto &&thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(queue_->empty());
  EXPECT_TRUE(queue_->IsValid());
}

TEST_F(QueueFixture, Front_AfterPushFronts_ReadPushedItems)
{
  PushElements(kRepeatNum);
  for (size_t i = 0; i < kRepeatNum; ++i) {
    EXPECT_EQ(queue_->front(), i);
    queue_->pop();
  }
}

TEST_F(QueueFixture, Back_AfterPushFronts_ReadPushedItems)
{
  for (size_t i = 0; i < kRepeatNum; ++i) {
    queue_->push(i);
    EXPECT_EQ(queue_->back(), i);
  }
}

TEST_F(QueueFixture, PushAndPop_EmptyQueue_QueueKeepValid)
{
  std::vector<std::thread> threads;
  size_t i = 0;
  for (; i < kThreadNum; i += 2) {
    threads.emplace_back(&QueueFixture::PopElements, this, kRepeatNum);
    threads.emplace_back(&QueueFixture::PushElements, this, kRepeatNum);
  }
  for (auto &&thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(queue_->IsValid());
}
