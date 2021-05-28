// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

using T = uint64_t;

static constexpr auto mo_relax = std::memory_order_relaxed;

static constexpr size_t kGCInterval = 1E3;
