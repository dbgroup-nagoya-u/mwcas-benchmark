// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "deque.hpp"
#include "mwcas/mwcas_descriptor.hpp"

namespace dbgroup::container
{
using ::dbgroup::atomic::mwcas::MwCASDescriptor;
using ::dbgroup::atomic::mwcas::ReadMwCASField;

class DequeMwCAS : public Deque
{
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  DequeMwCAS() : Deque{} {}

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  T
  Front() override
  {
    return ReadMwCASField<Node*>(front_.next)->elem;
  }

  T
  Back() override
  {
    return ReadMwCASField<Node*>(back_.prev)->elem;
  }

  void
  PushFront(T&& x) override
  {
    auto old_node = ReadMwCASField<Node*>(front_.next);
    auto new_node = new Node{T{x}, &front_, old_node};

    do {
      MwCASDescriptor desc{};
      desc.AddMwCASTarget(&front_.next, old_node, new_node);
      desc.AddMwCASTarget(&old_node->prev, &front_, new_node);

      if (desc.MwCAS()) {
        break;
      } else {
        old_node = ReadMwCASField<Node*>(front_.next);
        new_node->next = old_node;
      }
    } while (true);
  }

  void
  PushBack(T&& x) override
  {
    auto old_node = ReadMwCASField<Node*>(back_.prev);
    auto new_node = new Node{T{x}, old_node, &back_};

    do {
      MwCASDescriptor desc{};
      desc.AddMwCASTarget(&back_.prev, old_node, new_node);
      desc.AddMwCASTarget(&old_node->next, &back_, new_node);

      if (desc.MwCAS()) {
        break;
      } else {
        old_node = ReadMwCASField<Node*>(back_.prev);
        new_node->prev = old_node;
      }
    } while (true);
  }
};

}  // namespace dbgroup::container
