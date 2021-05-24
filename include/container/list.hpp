// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "common.hpp"

namespace dbgroup::container
{
class List
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

  List() : front_{T{}, &back_, nullptr}, back_{T{}, &front_, nullptr} {}

  virtual ~List()
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

  virtual T Front() = delete;

  virtual T Back() = delete;

  virtual void PushFront(T&& x) = delete;

  virtual void PushBack(T&& x) = delete;
};

}  // namespace dbgroup::container
