// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include "container/deque_mwcas.hpp"

#include "gtest/gtest.h"

class DequeMwCASFixture : public ::testing::Test
{
  using Key = uint64_t;
  using Deque_t = ::dbgroup::container::DequeMwCAS;

 public:
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
