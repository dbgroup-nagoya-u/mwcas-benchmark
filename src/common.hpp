// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <cstdint>

enum Operation
{
  kRead,
  kWrite
};

enum BenchTarget
{
  kOurs,
  kMicrosoft,
  kSingleCAS
};

constexpr size_t kMaxTargetNum = 8;
