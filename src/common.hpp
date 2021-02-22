// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <cstdint>

enum OperationType
{
  kRead,
  kWrite
};

enum BenchTarget
{
  kOurs,
  kMicrosoft
};

constexpr size_t kFieldNum = 16;
