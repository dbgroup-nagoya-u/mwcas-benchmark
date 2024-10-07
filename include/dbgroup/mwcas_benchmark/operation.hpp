/*
 * Copyright 2021 Database Group, Nagoya University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DBGROUP_MWCAS_BENCHMARK_OPERATION_HPP_
#define DBGROUP_MWCAS_BENCHMARK_OPERATION_HPP_

// C++ standard libraries
#include <cstddef>
#include <vector>

namespace dbgroup
{
/**
 * @brief A class for representing benchmarking operation.
 *
 */
class Operation
{
 public:
  /*############################################################################
   * Public constructors and assignment operators
   *##########################################################################*/

  constexpr Operation() = default;

  Operation(const Operation &) = default;
  Operation(Operation &&) = default;

  Operation &operator=(const Operation &obj) = default;
  Operation &operator=(Operation &&) = default;

  /*############################################################################
   * Public destructors
   *##########################################################################*/

  ~Operation() = default;

  /*############################################################################
   * Public APIs
   *##########################################################################*/

  /**
   * @return The operation ID.
   */
  [[nodiscard]] constexpr auto
  GetOpsID() const  //
      -> size_t
  {
    return 0;
  }

  /**
   * @return The target positions in an array.
   */
  [[nodiscard]] constexpr auto
  GetPositions() const  //
      -> const std::vector<size_t> &
  {
    return targets_;
  }

  /**
   * @brief Set the position of an element as i-th target.
   *
   * @param pos The position in an array.
   * @retval true if the position has been set.
   * @retval false otherwise.
   * @note This function checks the uniqueness of given positions for
   * guaranteeing linearizability of PMwCAS operations.
   */
  auto SetPositionIfUnique(  //
      size_t pos)            //
      -> bool;
  /**
   * @brief Sort target positions to linearize PMwCAS operations.
   *
   */
  void SortTargets();

 private:
  /*############################################################################
   * Internal member variables
   *##########################################################################*/

  /// @brief Target positions of an MwCAS operation
  std::vector<size_t> targets_{};
};

}  // namespace dbgroup

#endif  // DBGROUP_MWCAS_BENCHMARK_OPERATION_HPP_
