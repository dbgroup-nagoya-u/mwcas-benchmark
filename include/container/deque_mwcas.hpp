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
 public:
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
    return ReadMwCASField<Node*>(&front_.next)->elem;
  }

  T
  Back() override
  {
    return ReadMwCASField<Node*>(&back_.prev)->elem;
  }

  void
  PushFront(T&& x) override
  {
    auto old_node = ReadMwCASField<Node*>(&front_.next);
    auto new_node = new Node{T{x}, old_node, &front_};

    while (true) {
      MwCASDescriptor desc{};
      desc.AddMwCASTarget(&front_.next, old_node, new_node);
      desc.AddMwCASTarget(&(old_node->prev), &front_, new_node);

      if (desc.MwCAS()) {
        break;
      } else {
        old_node = ReadMwCASField<Node*>(&front_.next);
        new_node->next = old_node;
      }
    }
  }

  void
  PushBack(T&& x) override
  {
    auto old_node = ReadMwCASField<Node*>(&back_.prev);
    auto new_node = new Node{T{x}, &back_, old_node};

    while (true) {
      MwCASDescriptor desc{};
      desc.AddMwCASTarget(&(old_node->next), &back_, new_node);
      desc.AddMwCASTarget(&back_.prev, old_node, new_node);

      if (desc.MwCAS()) {
        break;
      } else {
        old_node = ReadMwCASField<Node*>(&back_.prev);
        new_node->prev = old_node;
      }
    }
  }

  void
  PopFront() override
  {
    auto old_node = ReadMwCASField<Node*>(&front_.next);
    auto new_node = ReadMwCASField<Node*>(&(old_node->next));

    while (new_node != nullptr) {  // if new_node is null, old_node is end of a deque
      MwCASDescriptor desc{};
      desc.AddMwCASTarget(&front_.next, old_node, new_node);
      desc.AddMwCASTarget(&(new_node->prev), old_node, &front_);

      if (desc.MwCAS()) {
        delete old_node;
        break;
      } else {
        old_node = ReadMwCASField<Node*>(&front_.next);
        new_node = ReadMwCASField<Node*>(&(old_node->next));
      }
    }
  }

  void
  PopBack() override
  {
    auto old_node = ReadMwCASField<Node*>(&back_.prev);
    auto new_node = ReadMwCASField<Node*>(&(old_node->prev));

    while (new_node != nullptr) {  // if new_node is null, old_node is end of a deque
      MwCASDescriptor desc{};
      desc.AddMwCASTarget(&(new_node->next), old_node, &back_);
      desc.AddMwCASTarget(&back_.prev, old_node, new_node);

      if (desc.MwCAS()) {
        delete old_node;
        break;
      } else {
        old_node = ReadMwCASField<Node*>(&back_.prev);
        new_node = ReadMwCASField<Node*>(&(old_node->prev));
      }
    }
  }

  bool
  Empty() override
  {
    const auto next_node = ReadMwCASField<Node*>(&front_.next);
    return ReadMwCASField<Node*>(&(next_node->next)) == nullptr;
  }
};

}  // namespace dbgroup::container
