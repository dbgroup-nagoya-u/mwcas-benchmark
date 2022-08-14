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

#include <future>
#include <memory>
#include <thread>
#include <vector>

// external libraries
#include "gtest/gtest.h"

// local sources
#include "queue/queue_cas.hpp"
#include "queue/queue_mutex.hpp"
#include "queue/queue_mwcas.hpp"

namespace dbgroup::container::test
{
/*######################################################################################
 * Global constants
 *####################################################################################*/

constexpr size_t kRepeatNum = 1E5;
constexpr size_t kThreadNum = 8;

/*######################################################################################
 * Fixture Classes
 *####################################################################################*/

template <class Queue>
class QueueFixture : public ::testing::Test
{
 protected:
  /*####################################################################################
   * SetUp/TearDown
   *##################################################################################*/

  void
  SetUp()
  {
    queue_ = std::make_unique<Queue>();
  }

  void
  TearDown()
  {
    queue_.reset(nullptr);
  }

  /*####################################################################################
   * Internal utilities
   *##################################################################################*/

  void
  PushElements()
  {
    for (size_t i = 0; i < kRepeatNum; ++i) {
      queue_->push(1UL);
    }
  }

  auto
  PopElements()  //
      -> size_t
  {
    size_t sum = 0;
    while (true) {
      const auto &elem = queue_->pop();
      if (!elem) break;

      sum += *elem;
    }

    return sum;
  }

  /*####################################################################################
   * Verifications
   *##################################################################################*/

  void
  VerifyWithSingleThread()
  {
    PushElements();
    const auto sum = PopElements();

    EXPECT_EQ(kRepeatNum, sum);
  }

  void
  VerifyWithMultiThreads()
  {
    // push elements with multi threads
    std::vector<std::thread> threads{};
    for (size_t i = 0; i < kThreadNum; ++i) {
      threads.emplace_back(&QueueFixture::PushElements, this);
    }
    for (auto &&t : threads) {
      t.join();
    }

    // pop elements with multi threads
    auto pop_func = [&](std::promise<size_t> p) { p.set_value(PopElements()); };
    std::vector<std::future<size_t>> futures{};
    for (size_t i = 0; i < kThreadNum; ++i) {
      std::promise<size_t> p{};
      futures.emplace_back(p.get_future());
      std::thread{pop_func, std::move(p)}.detach();
    }

    // summarize popped elements
    size_t sum = 0;
    for (auto &&f : futures) {
      sum += f.get();
    }

    EXPECT_EQ(kRepeatNum * kThreadNum, sum);
  }

  /*####################################################################################
   * Internal member vairables
   *##################################################################################*/

  std::unique_ptr<Queue> queue_ = nullptr;
};

/*######################################################################################
 * Preparation for typed testing
 *####################################################################################*/

using TestTargets = ::testing::Types<  //
    QueueMutex<size_t>,
    QueueCAS<size_t>,
    QueueMwCAS<size_t>  //
    >;
TYPED_TEST_SUITE(QueueFixture, TestTargets);

/*######################################################################################
 * Unit test definitions
 *####################################################################################*/

TYPED_TEST(QueueFixture, PushPopWithSingleThreadRunConsistently)
{  //
  TestFixture::VerifyWithSingleThread();
}

TYPED_TEST(QueueFixture, PushPopWithMultiThreadsRunConsistently)
{  //
  TestFixture::VerifyWithMultiThreads();
}

}  // namespace dbgroup::container::test
