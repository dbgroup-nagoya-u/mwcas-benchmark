/*
 * Copyright 2022 Database Group, Nagoya University
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

#ifndef MWCAS_BENCHMARK_QUEUE_QUEUE_MUTEX_H
#define MWCAS_BENCHMARK_QUEUE_QUEUE_MUTEX_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <shared_mutex>

namespace dbgroup::container
{
/**
 * @brief A class to implement a thread-safe queue by using C++ mutex library.
 *
 */
template <class T>
class QueueMutex
{
 public:
  /*####################################################################################
   * Public constructors/destructors
   *##################################################################################*/

  /**
   * @brief Construct a new QueueMutex object.
   *
   * The object uses C++ mutex library to perform thread-safe push/pop operations.
   */
  QueueMutex() = default;

  ~QueueMutex()
  {
    while (!empty()) {
      pop();
    }
    delete front_;
  }

  /*####################################################################################
   * Public utility functions
   *##################################################################################*/

  void
  push(const T x)
  {
    auto *new_node = new Node{x, nullptr};

    std::unique_lock<std::shared_mutex> guard{mtx_};

    back_->next = new_node;
    back_ = new_node;
  }

  auto
  pop()  //
      -> std::optional<T>
  {
    std::unique_lock<std::shared_mutex> guard{mtx_};

    auto *head_node = front_->next;
    if (head_node == nullptr) return std::nullopt;

    delete front_;
    front_ = head_node;

    return front_->elem;
  }

  auto
  empty()  //
      -> bool
  {
    std::shared_lock<std::shared_mutex> guard{mtx_};

    return front_->next == nullptr;
  }

 private:
  /*####################################################################################
   * Internal classes
   *##################################################################################*/

  /**
   * @brief A class to represent nodes in a queue.
   *
   */
  struct Node {
    /// an element of a queue
    const T elem{};

    /// a previous node of a queue
    Node *next{nullptr};
  };

  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  /// a pointer to the front (i.e., oldest element) of a queue.
  Node *front_{new Node{}};

  /// a pointer to the back (i.e., newest element) of a queue.
  Node *back_{front_};

  /// a mutex object for global locking.
  std::shared_mutex mtx_{};
};

}  // namespace dbgroup::container

#endif  // MWCAS_BENCHMARK_QUEUE_QUEUE_MUTEX_H
