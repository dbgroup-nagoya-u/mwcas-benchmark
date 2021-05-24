// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include "container/deque_mwcas.hpp"

#include "gtest/gtest.h"

class DequeMwCASFixture : public ::testing::Test
{
 public:
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

TEST_F(DequeMwCASFixture, MeasureThroughput_SwapSameFields_ReadCorrectValues) {}
