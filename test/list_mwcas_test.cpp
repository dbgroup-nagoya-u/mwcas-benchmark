// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include "container/list_mwcas.hpp"

#include "gtest/gtest.h"

class ListMwCASFixture : public ::testing::Test
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

TEST_F(ListMwCASFixture, MeasureThroughput_SwapSameFields_ReadCorrectValues) {}
