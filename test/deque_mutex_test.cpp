// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include "container/deque_mutex.hpp"

using Key = uint64_t;
using Deque_t = ::dbgroup::container::DequeMutex;

#include "deque_test.hpp"

void
DequeFixture::SetUp()
{
  deque_ = std::make_unique<Deque_t>();
}

void
DequeFixture::TearDown()
{
}
