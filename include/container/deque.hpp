// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "common.hpp"

namespace dbgroup::container
{
/**
 * @brief An abstract class to represent a thread-safe deque.
 *
 */
class Deque
{
 protected:
  /**
   * @brief A class to represent nodes in a deque.
   *
   */
  struct Node {
    /// an element of a deque
    const T elem = T{};

    /// a next node of a deque
    Node* next = nullptr;

    /// a previous node of a deque
    Node* prev = nullptr;
  };

  /// a dummy node to represent the head of a deque
  Node front_;

  /// a dummy node to represent the tail of a deque
  Node back_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  /**
   * @brief Construct a new Deque object.
   *
   */
  Deque() : front_{T{}, &back_, nullptr}, back_{T{}, nullptr, &front_} {}

  /**
   * @brief Destroy the Deque object.
   *
   * Since the Deque object comprises Node objects, the destructor deletes all of them.
   */
  virtual ~Deque()
  {
    auto next = front_.next;
    while (next->next != nullptr) {
      auto prev = next;
      next = next->next;
      delete prev;
    }
  }

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  /**
   * @brief Get a front element from the deque.
   *
   * Note that if the deque is empty, the return value is undefined.
   *
   * @return T a front element
   */
  virtual T Front() = 0;

  /**
   * @brief Get a back element from the deque.
   *
   * Note that if the deque is empty, the return value is undefined.
   *
   * @return T a back element
   */
  virtual T Back() = 0;

  /**
   * @brief Push a new element to the front of the deque.
   *
   * @param x a new element to push
   */
  virtual void PushFront(const T x) = 0;

  /**
   * @brief Push a new element to the back of the deque.
   *
   * @param x a new element to push
   */
  virtual void PushBack(const T x) = 0;

  /**
   * @brief Pop a front element.
   *
   * Note that if the deque has no element, this method do nothing.
   */
  virtual void PopFront() = 0;

  /**
   * @brief Pop a back element.
   *
   * Note that if the deque has no element, this method do nothing.
   */
  virtual void PopBack() = 0;

  /**
   * @retval true if the deque is empty
   * @retval false if the deque has any elements
   */
  virtual bool Empty() = 0;
};

}  // namespace dbgroup::container
