// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <memory>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

class DequeFixture : public ::testing::Test
{
 public:
  static constexpr size_t kRepeatNum = 1E5;
  static constexpr size_t kThreadNum = 8;

  std::unique_ptr<Deque_t> deque_ = nullptr;

  void
  PushFronts(const size_t repeat_num)
  {
    for (size_t i = 0; i < repeat_num; ++i) {
      deque_->PushFront(i);
    }
  }

  void
  PopBacks(const size_t repeat_num)
  {
    for (size_t i = 0; i < repeat_num; ++i) {
      deque_->PopBack();
    }
  }

 protected:
  void SetUp();

  void TearDown();
};

TEST_F(DequeFixture, Construct_Default_DequeCorrectlyInitialized)
{
  EXPECT_TRUE(deque_->Empty());
  EXPECT_TRUE(deque_->IsValid());
}

TEST_F(DequeFixture, PushFront_OneItem_DequeIsNotEmpty)
{
  deque_->PushFront(0);
  EXPECT_FALSE(deque_->Empty());
  EXPECT_TRUE(deque_->IsValid());
}

TEST_F(DequeFixture, PushFront_ByMultiThreads_AllItemsPushed)
{
  std::vector<std::thread> threads;
  for (size_t i = 0; i < kThreadNum; ++i) {
    threads.emplace_back(&DequeFixture::PushFronts, this, kRepeatNum);
  }
  for (auto &&thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(deque_->IsValid());

  for (size_t i = 0; i < kRepeatNum * kThreadNum; ++i) {
    EXPECT_FALSE(deque_->Empty());
    deque_->PopBack();
  }
  EXPECT_TRUE(deque_->Empty());
}

TEST_F(DequeFixture, PopBack_AfterPushFront_DequeIsEmpty)
{
  deque_->PushFront(0);
  deque_->PopBack();
  EXPECT_TRUE(deque_->Empty());
  EXPECT_TRUE(deque_->IsValid());
}

TEST_F(DequeFixture, PopBack_AfterPushBack_DequeIsEmpty)
{
  deque_->PushBack(0);
  deque_->PopBack();
  EXPECT_TRUE(deque_->Empty());
  EXPECT_TRUE(deque_->IsValid());
}

TEST_F(DequeFixture, PopBack_ByMultiThreads_AllItemsPopped)
{
  PushFronts(kRepeatNum * kThreadNum);

  std::vector<std::thread> threads;
  for (size_t i = 0; i < kThreadNum; ++i) {
    threads.emplace_back(&DequeFixture::PopBacks, this, kRepeatNum);
  }
  for (auto &&thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(deque_->Empty());
  EXPECT_TRUE(deque_->IsValid());
}

TEST_F(DequeFixture, Front_AfterPushFronts_ReadPushedItems)
{
  for (size_t i = 0; i < kRepeatNum; ++i) {
    deque_->PushFront(i);
    EXPECT_EQ(deque_->Front(), i);
  }
}

TEST_F(DequeFixture, Back_AfterPushFronts_ReadPushedItems)
{
  PushFronts(kRepeatNum);
  for (size_t i = 0; i < kRepeatNum; ++i) {
    EXPECT_EQ(deque_->Back(), i);
    deque_->PopBack();
  }
}

TEST_F(DequeFixture, PushAndPop_EmptyDeque_QueueKeepValid)
{
  std::vector<std::thread> threads;
  size_t i = 0;
  for (; i < kThreadNum; i += 2) {
    threads.emplace_back(&DequeFixture::PopBacks, this, kRepeatNum);
    threads.emplace_back(&DequeFixture::PushFronts, this, kRepeatNum);
  }
  for (auto &&thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(deque_->IsValid());
}
