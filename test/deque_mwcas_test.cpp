// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include "container/deque_mwcas.hpp"

#include <thread>
#include <vector>

#include "gtest/gtest.h"

class DequeMwCASFixture : public ::testing::Test
{
  using Key = uint64_t;
  using Deque_t = ::dbgroup::container::DequeMwCAS;

 public:
  static constexpr size_t kRepeatNum = 1E5;
  static constexpr size_t kThreadNum = 8;

  Deque_t deque_;

  void
  PushFronts(const size_t repeat_num)
  {
    for (size_t i = 0; i < repeat_num; ++i) {
      deque_.PushFront(i);
    }
  }

  void
  PushBacks(const size_t repeat_num)
  {
    for (size_t i = 0; i < repeat_num; ++i) {
      deque_.PushBack(i);
    }
  }

  void
  PopFronts(const size_t repeat_num)
  {
    for (size_t i = 0; i < repeat_num; ++i) {
      deque_.PopFront();
    }
  }

  void
  PopBacks(const size_t repeat_num)
  {
    for (size_t i = 0; i < repeat_num; ++i) {
      deque_.PopBack();
    }
  }

 protected:
  void
  SetUp() override
  {
  }

  void
  TearDown() override
  {
  }
};

TEST_F(DequeMwCASFixture, Construct_Default_DequeCorrectlyInitialized)
{
  EXPECT_TRUE(deque_.Empty());
}

TEST_F(DequeMwCASFixture, PushFront_OneItem_DequeIsNotEmpty)
{
  deque_.PushFront(0);
  EXPECT_FALSE(deque_.Empty());
}

TEST_F(DequeMwCASFixture, PushFront_ByMultiThreads_AllItemsPushed)
{
  std::vector<std::thread> threads;
  for (size_t i = 0; i < kThreadNum; ++i) {
    threads.emplace_back(&DequeMwCASFixture::PushFronts, this, kRepeatNum);
  }
  for (auto &&thread : threads) {
    thread.join();
  }

  for (size_t i = 0; i < kRepeatNum * kThreadNum; ++i) {
    EXPECT_FALSE(deque_.Empty());
    deque_.PopFront();
  }
  EXPECT_TRUE(deque_.Empty());
}

TEST_F(DequeMwCASFixture, PopFront_AfterPushFront_DequeIsEmpty)
{
  deque_.PushFront(0);
  deque_.PopFront();
  EXPECT_TRUE(deque_.Empty());
}

TEST_F(DequeMwCASFixture, PopFront_AfterPushBack_DequeIsEmpty)
{
  deque_.PushBack(0);
  deque_.PopFront();
  EXPECT_TRUE(deque_.Empty());
}

TEST_F(DequeMwCASFixture, PopFront_ByMultiThreads_AllItemsPopped)
{
  PushFronts(kRepeatNum * kThreadNum);

  std::vector<std::thread> threads;
  for (size_t i = 0; i < kThreadNum; ++i) {
    threads.emplace_back(&DequeMwCASFixture::PopFronts, this, kRepeatNum);
  }
  for (auto &&thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(deque_.Empty());
}

TEST_F(DequeMwCASFixture, PushBack_OneItem_DequeIsNotEmpty)
{
  deque_.PushBack(0);
  EXPECT_FALSE(deque_.Empty());
}

TEST_F(DequeMwCASFixture, PushBack_ByMultiThreads_AllItemsPushed)
{
  std::vector<std::thread> threads;
  for (size_t i = 0; i < kThreadNum; ++i) {
    threads.emplace_back(&DequeMwCASFixture::PushBacks, this, kRepeatNum);
  }
  for (auto &&thread : threads) {
    thread.join();
  }

  for (size_t i = 0; i < kRepeatNum * kThreadNum; ++i) {
    EXPECT_FALSE(deque_.Empty());
    deque_.PopFront();
  }
  EXPECT_TRUE(deque_.Empty());
}

TEST_F(DequeMwCASFixture, PopBack_AfterPushFront_DequeIsEmpty)
{
  deque_.PushFront(0);
  deque_.PopBack();
  EXPECT_TRUE(deque_.Empty());
}

TEST_F(DequeMwCASFixture, PopBack_AfterPushBack_DequeIsEmpty)
{
  deque_.PushBack(0);
  deque_.PopBack();
  EXPECT_TRUE(deque_.Empty());
}

TEST_F(DequeMwCASFixture, PopBack_ByMultiThreads_AllItemsPopped)
{
  PushFronts(kRepeatNum * kThreadNum);

  std::vector<std::thread> threads;
  for (size_t i = 0; i < kThreadNum; ++i) {
    threads.emplace_back(&DequeMwCASFixture::PopBacks, this, kRepeatNum);
  }
  for (auto &&thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(deque_.Empty());
}

TEST_F(DequeMwCASFixture, Front_AfterPushFronts_ReadPushedItems)
{
  for (size_t i = 0; i < kRepeatNum; ++i) {
    deque_.PushFront(i);
    EXPECT_EQ(deque_.Front(), i);
  }
}

TEST_F(DequeMwCASFixture, Front_AfterPushBacks_ReadPushedItems)
{
  PushBacks(kRepeatNum);
  for (size_t i = 0; i < kRepeatNum; ++i) {
    EXPECT_EQ(deque_.Front(), i);
    deque_.PopFront();
  }
}

TEST_F(DequeMwCASFixture, Back_AfterPushBacks_ReadPushedItems)
{
  for (size_t i = 0; i < kRepeatNum; ++i) {
    deque_.PushBack(i);
    EXPECT_EQ(deque_.Back(), i);
  }
}

TEST_F(DequeMwCASFixture, Back_AfterPushFronts_ReadPushedItems)
{
  PushFronts(kRepeatNum);
  for (size_t i = 0; i < kRepeatNum; ++i) {
    EXPECT_EQ(deque_.Back(), i);
    deque_.PopBack();
  }
}
