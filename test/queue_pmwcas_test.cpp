/*
 * Copyright 2021 Database Group, Nagoya University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
