// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include "container/deque_mwcas.hpp"

using Key = uint64_t;
using Queue_t = ::dbgroup::container::QueueMwCAS;

#include "deque_test.hpp"

void
QueueFixture::SetUp()
{
  queue_ = std::make_unique<Queue_t>();
}

void
QueueFixture::TearDown()
{
}
