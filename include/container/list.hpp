// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "common.hpp"
#include "node.hpp"

namespace dbgroup::container
{
class List
{
 protected:
  list::Node front_;

  list::Node back_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  List() {}

  virtual ~List() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  virtual T Front() = delete;

  virtual T Back() = delete;

  virtual void PushFront(T&& x) = delete;

  virtual void PushBack(T&& x) = delete;
};

}  // namespace dbgroup::container
