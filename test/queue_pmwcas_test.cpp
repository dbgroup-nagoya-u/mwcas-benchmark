// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#include "container/queue_pmwcas.hpp"

using Key = uint64_t;
using Queue_t = ::dbgroup::container::QueuePMwCAS;

#include "queue_test.hpp"

void
QueueFixture::SetUp()
{
  queue_ = std::make_unique<Queue_t>(kThreadNum);
}

void
QueueFixture::TearDown()
{
}