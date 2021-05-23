// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "common.hpp"

namespace dbgroup::container
{
class ListElement
{
 private:
  const T elem_;

  ListElement *next_;

  ListElement *prev_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  explicit ListElement(const T &&x) : elem_{x}, next_{nullptr}, prev_{nullptr} {}

  ~ListElement() = default;

  /*################################################################################################
   * Public getters/setters
   *##############################################################################################*/

  T
  Get() const
  {
    return elem_;
  }

  ListElement *
  Next() const
  {
    return next_;
  }

  ListElement *
  Prev() const
  {
    return prev_;
  }
};

}  // namespace dbgroup::container
