// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "common.hpp"

namespace dbgroup::container
{
class Deque
{
 protected:
  struct Node {
    const T elem = T{};

    Node* next = nullptr;

    Node* prev = nullptr;
  };

  Node front_;

  Node back_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  Deque() : front_{T{}, &back_, nullptr}, back_{T{}, nullptr, &front_} {}

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

  virtual T Front() = 0;

  virtual T Back() = 0;

  virtual void PushFront(T&& x) = 0;

  virtual void PushBack(T&& x) = 0;

  virtual void PopFront() = 0;

  virtual void PopBack() = 0;

  virtual bool Empty() = 0;
};

}  // namespace dbgroup::container
