// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "common.hpp"

namespace dbgroup::container
{
/**
 * @brief An abstract class to represent a thread-safe queue.
 *
 */
class Queue
{
 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  /**
   * @brief Construct a new Queue object.
   *
   */
  Queue() = default;

  /**
   * @brief Destroy the Queue object.
   *
   * Since the Queue object comprises Node objects, the destructor deletes all of them.
   */
  virtual ~Queue() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  /**
   * @brief Get a front element from the queue.
   *
   * Note that if the queue is empty, the return value is undefined.
   *
   * @return T a front element
   */
  virtual T front() = 0;

  /**
   * @brief Get a back element from the queue.
   *
   * Note that if the queue is empty, the return value is undefined.
   *
   * @return T a back element
   */
  virtual T back() = 0;

  /**
   * @brief Push a new element to the back of the queue.
   *
   * @param x a new element to push
   */
  virtual void push(const T x) = 0;

  /**
   * @brief Pop the front element.
   *
   * Note that if the queue has no element, this method do nothing.
   */
  virtual void pop() = 0;

  /**
   * @retval true if the queue is empty
   * @retval false if the queue has any elements
   */
  virtual bool empty() = 0;

  virtual bool IsValid() const = 0;
};

}  // namespace dbgroup::container
