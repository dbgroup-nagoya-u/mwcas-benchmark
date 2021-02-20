// Copyright (c) Database Group, Nagoya University. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <cstdint>
#include <vector>

class Worker
{
 private:
  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  size_t read_ratio_;

  size_t operation_counts_;

  std::vector<size_t> exec_times_nano_;

  std::vector<size_t *> shared_fields_;

  std::vector<size_t> private_fields_;

 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  Worker(  //
      const size_t private_field_num,
      const std::vector<size_t *> shared_fields,
      const size_t read_ratio,
      const size_t operation_counts)
      : read_ratio_{read_ratio}, operation_counts_{operation_counts}
  {
    exec_times_nano_.reserve(operation_counts_);
    shared_fields_.insert(shared_fields_.begin(), shared_fields.begin(), shared_fields.end());
    for (size_t count = 0; count < private_field_num; ++count) {
      private_fields_.emplace_back(0);
    }
  }

  ~Worker() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  virtual size_t ReadMwCASField(const void *target_addr);

  virtual size_t PerformMwCAS(  //
      const std::vector<size_t *> &shared_fields,
      const std::vector<size_t> &private_fields);
};
