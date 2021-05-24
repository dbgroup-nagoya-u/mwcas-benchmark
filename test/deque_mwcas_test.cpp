// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include "container/deque_mwcas.hpp"

#include "gtest/gtest.h"

class DequeMwCASFixture : public ::testing::Test
{
  using Key = uint64_t;
  using Deque_t = ::dbgroup::container::DequeMwCAS;

 public:
  static constexpr size_t kRepeatCount = 1E5;

  Deque_t deque_;

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

TEST_F(DequeMwCASFixture, PushBack_OneItem_DequeIsNotEmpty)
{
  deque_.PushBack(0);
  EXPECT_FALSE(deque_.Empty());
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

TEST_F(DequeMwCASFixture, Front_AfterPushFronts_ReadPushedItems)
{
  for (size_t i = 0; i < kRepeatCount; ++i) {
    deque_.PushFront(i);
    EXPECT_EQ(deque_.Front(), i);
  }
}

TEST_F(DequeMwCASFixture, Front_AfterPushBacks_ReadPushedItems)
{
  for (size_t i = 0; i < kRepeatCount; ++i) {
    deque_.PushBack(i);
  }
  for (size_t i = 0; i < kRepeatCount; ++i) {
    EXPECT_EQ(deque_.Front(), i);
    deque_.PopFront();
  }
}

TEST_F(DequeMwCASFixture, Back_AfterPushBacks_ReadPushedItems)
{
  for (size_t i = 0; i < kRepeatCount; ++i) {
    deque_.PushBack(i);
    EXPECT_EQ(deque_.Back(), i);
  }
}

TEST_F(DequeMwCASFixture, Back_AfterPushFronts_ReadPushedItems)
{
  for (size_t i = 0; i < kRepeatCount; ++i) {
    deque_.PushFront(i);
  }
  for (size_t i = 0; i < kRepeatCount; ++i) {
    EXPECT_EQ(deque_.Back(), i);
    deque_.PopBack();
  }
}
