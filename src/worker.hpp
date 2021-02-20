// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <array>
#include <cstdint>
#include <vector>

class Worker
{
 private:
  /*################################################################################################
   * Internal enum and constants
   *##############################################################################################*/

  static constexpr auto kPrivateFieldNum = 128UL;

  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  size_t read_ratio_;

  size_t operation_counts_;

  std::vector<size_t> exec_times_nano_;

 protected:
  /*################################################################################################
   * Inherited member variables
   *##############################################################################################*/

  std::array<size_t, kPrivateFieldNum> private_fields_;

  std::vector<size_t*> shared_fields_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  Worker(  //
      const std::vector<size_t*> shared_fields,
      const size_t read_ratio,
      const size_t operation_counts)
      : read_ratio_{read_ratio}, operation_counts_{operation_counts}
  {
    private_fields_.fill(0);
    shared_fields_.insert(shared_fields_.begin(), shared_fields.begin(), shared_fields.end());
    exec_times_nano_.reserve(operation_counts_);
  }

  ~Worker() = default;
};
