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

#ifndef DBGROUP_MWCAS_BENCHMARK_OPERATION_ENGINE_HPP_
#define DBGROUP_MWCAS_BENCHMARK_OPERATION_ENGINE_HPP_

// C++ standard libraries
#include <cstddef>
#include <vector>

// external libraries
#include "dbgroup/random/zipf.hpp"

// local sources
#include "dbgroup/mwcas_benchmark/operation.hpp"

namespace dbgroup
{
class OperationEngine
{
  /*############################################################################
   * Type aliases
   *##########################################################################*/

  using ZipfDist_t = ::dbgroup::random::ApproxZipfDistribution<size_t>;

 public:
  /*############################################################################
   * Public constructors and assignment operators
   *##########################################################################*/

  /**
   * @brief Construct a new OperationEngine object.
   *
   * @param target_num The number of target words fow MwCAS.
   * @param array_cap The capacity of an array.
   * @param skew_param A skew parameter in Zipf's law.
   * @param random_seed A seed value for reproducibility.
   */
  OperationEngine(  //
      size_t target_num,
      size_t array_cap,
      double skew_param,
      size_t random_seed);

  OperationEngine(const OperationEngine &) = default;
  OperationEngine(OperationEngine &&) = default;

  OperationEngine &operator=(const OperationEngine &obj) = default;
  OperationEngine &operator=(OperationEngine &&) = default;

  /*############################################################################
   * Public destructors
   *##########################################################################*/

  ~OperationEngine() = default;

  /*############################################################################
   * Public APIs
   *##########################################################################*/

  /**
   * @return The number of target operation types.
   */
  [[nodiscard]] constexpr auto
  GetOpsTypeNum() const  //
      -> size_t
  {
    return 1;
  }

  /**
   * @param n The number of operations to be executed by each worker.
   * @param random_seed A seed value for reproducibility.
   * @return A sequence of operations for MwCAS.
   */
  [[nodiscard]] auto Generate(  //
      size_t n,
      size_t random_seed) const  //
      -> std::vector<Operation>;

 private:
  /*############################################################################
   * Internal member variables
   *##########################################################################*/

  /// @brief The index for indicating actual positions in an array.
  std::vector<size_t> pos_index_{};

  /// @brief The number of target words for MwCAS.
  size_t target_num_{};

  /// @brief A random value generator according to Zipf's law.
  ZipfDist_t zipf_dist_{};
};

}  // namespace dbgroup

#endif  // DBGROUP_MWCAS_BENCHMARK_OPERATION_ENGINE_HPP_
