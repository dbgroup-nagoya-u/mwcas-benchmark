// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "common.hpp"

namespace dbgroup::container::list
{
class Node
{
 private:
  const T elem_;

  Node *next_;

  Node *prev_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  Node() : elem_{}, next_{nullptr}, prev_{nullptr} {}

  explicit Node(const T &&x) : elem_{x}, next_{nullptr}, prev_{nullptr} {}

  ~Node() = default;

  /*################################################################################################
   * Public getters/setters
   *##############################################################################################*/

  T
  Get() const
  {
    return elem_;
  }

  Node *
  Next() const
  {
    return next_;
  }

  Node *
  Prev() const
  {
    return prev_;
  }
};

}  // namespace dbgroup::container::list
