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
  PushBacks(const size_t repeat_num)
  {
    for (size_t i = 0; i < repeat_num; ++i) {
      deque_->PushBack(i);
    }
  }

  void
  PopFronts(const size_t repeat_num)
  {
    for (size_t i = 0; i < repeat_num; ++i) {
      deque_->PopFront();
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

TEST_F(DequeFixture, Construct_Default_DequeCorrectlyInitialized) { EXPECT_TRUE(deque_->Empty()); }

TEST_F(DequeFixture, PushFront_OneItem_DequeIsNotEmpty)
{
  deque_->PushFront(0);
  EXPECT_FALSE(deque_->Empty());
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

  for (size_t i = 0; i < kRepeatNum * kThreadNum; ++i) {
    EXPECT_FALSE(deque_->Empty());
    deque_->PopFront();
  }
  EXPECT_TRUE(deque_->Empty());
}

TEST_F(DequeFixture, PopFront_AfterPushFront_DequeIsEmpty)
{
  deque_->PushFront(0);
  deque_->PopFront();
  EXPECT_TRUE(deque_->Empty());
}

TEST_F(DequeFixture, PopFront_AfterPushBack_DequeIsEmpty)
{
  deque_->PushBack(0);
  deque_->PopFront();
  EXPECT_TRUE(deque_->Empty());
}

TEST_F(DequeFixture, PopFront_ByMultiThreads_AllItemsPopped)
{
  PushFronts(kRepeatNum * kThreadNum);

  std::vector<std::thread> threads;
  for (size_t i = 0; i < kThreadNum; ++i) {
    threads.emplace_back(&DequeFixture::PopFronts, this, kRepeatNum);
  }
  for (auto &&thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(deque_->Empty());
}

TEST_F(DequeFixture, PushBack_OneItem_DequeIsNotEmpty)
{
  deque_->PushBack(0);
  EXPECT_FALSE(deque_->Empty());
}

TEST_F(DequeFixture, PushBack_ByMultiThreads_AllItemsPushed)
{
  std::vector<std::thread> threads;
  for (size_t i = 0; i < kThreadNum; ++i) {
    threads.emplace_back(&DequeFixture::PushBacks, this, kRepeatNum);
  }
  for (auto &&thread : threads) {
    thread.join();
  }

  for (size_t i = 0; i < kRepeatNum * kThreadNum; ++i) {
    EXPECT_FALSE(deque_->Empty());
    deque_->PopFront();
  }
  EXPECT_TRUE(deque_->Empty());
}

TEST_F(DequeFixture, PopBack_AfterPushFront_DequeIsEmpty)
{
  deque_->PushFront(0);
  deque_->PopBack();
  EXPECT_TRUE(deque_->Empty());
}

TEST_F(DequeFixture, PopBack_AfterPushBack_DequeIsEmpty)
{
  deque_->PushBack(0);
  deque_->PopBack();
  EXPECT_TRUE(deque_->Empty());
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
}

TEST_F(DequeFixture, Front_AfterPushFronts_ReadPushedItems)
{
  for (size_t i = 0; i < kRepeatNum; ++i) {
    deque_->PushFront(i);
    EXPECT_EQ(deque_->Front(), i);
  }
}

TEST_F(DequeFixture, Front_AfterPushBacks_ReadPushedItems)
{
  PushBacks(kRepeatNum);
  for (size_t i = 0; i < kRepeatNum; ++i) {
    EXPECT_EQ(deque_->Front(), i);
    deque_->PopFront();
  }
}

TEST_F(DequeFixture, Back_AfterPushBacks_ReadPushedItems)
{
  for (size_t i = 0; i < kRepeatNum; ++i) {
    deque_->PushBack(i);
    EXPECT_EQ(deque_->Back(), i);
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
