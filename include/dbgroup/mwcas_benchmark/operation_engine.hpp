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
#include <random>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

// external C++ libraries
#include <dbgroup/random/zipf.hpp>

namespace dbgroup
{
class OperationEngine
{
  /*##########################################################################*
   * Type aliases
   *##########################################################################*/

  using Zipf = ::dbgroup::random::ApproxZipfDistribution<size_t>;

 public:
  /*##########################################################################*
   * Public types
   *##########################################################################*/

  /**
   * @brief An enumeration for representing target operations.
   *
   * @note Our benchmark template requires this type.
   */
  enum OPType {
    kMwCAS = 0,
    kTotalNum,  /// @note This element is mandatory.
  };

  /// @note Our benchmark template requires this field.
  using Operation = std::vector<size_t>;

  /**
   * @brief A class for iterating an operation queue.
   *
   * @note Our benchmark template requires this type.
   */
  class OPIter
  {
   public:
    /*########################################################################*
     * Public constructors and assignment operators
     *########################################################################*/

    /**
     * @param target_num The number of target words fow MwCAS.
     * @param array_cap The capacity of an array.
     * @param skew_param A skew parameter in Zipf's law.
     * @param random_seed A seed value for reproducibility.
     * @param pos_index The index for indicating actual positions in an array.
     */
    OPIter(  //
        const size_t target_num,
        const size_t arr_cap,
        const double skew_parameter,
        const size_t rand_seed,
        Operation pos_index)
        : zipf_{0, arr_cap - 1, skew_parameter},
          rand_{rand_seed},
          target_num_{target_num},
          positions_(target_num, 0),
          pos_index_{std::move(pos_index)}
    {
      ++(*this);
    }

    OPIter(OPIter &&) noexcept = default;
    auto operator=(OPIter &&) noexcept -> OPIter & = default;

    // forbit copying
    OPIter(const OPIter &) = delete;
    auto operator=(const OPIter &obj) -> OPIter & = delete;

    /*########################################################################*
     * Public destructor
     *########################################################################*/

    ~OPIter() = default;

    /*########################################################################*
     * Public APIs
     *########################################################################*/

    /**
     * @retval true if this iterator has other operations.
     * @retval false otherwise.
     * @note Our benchmark template requires this operator.
     */
    [[nodiscard]]
    constexpr explicit
    operator bool() const
    {
      return true;
    }

    /**
     * @retval 1st: The current operation type.
     * @retval 2nd: Operation arguments.
     * @note Our benchmark template requires this operator.
     */
    [[nodiscard]]
    constexpr auto
    operator*() const  //
        -> std::pair<OPType, Operation>
    {
      return {type_, positions_};
    }

    /**
     * @brief Advance this iterator.
     *
     * @return Oneself.
     * @note Our benchmark template requires this operator.
     */
    auto operator++()  //
        -> OPIter &;

   private:
    /*########################################################################*
     * Internal member variables
     *########################################################################*/

    /// @brief A zipf distribution.
    Zipf zipf_{};

    /// @brief A random value generator.
    std::mt19937_64 rand_{};

    /// @brief The number of target words for MwCAS.
    size_t target_num_{};

    /// @brief The position of a target page.
    std::vector<size_t> positions_{};

    /// @brief The index for indicating actual positions in an array.
    Operation pos_index_{};

    /// @brief An operation type to be executed.
    /// @note Our benchmark template requires this field.
    OPType type_{};
  };

  /*##########################################################################*
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

  /*##########################################################################*
   * Public destructors
   *##########################################################################*/

  ~OperationEngine() = default;

  /*##########################################################################*
   * Public APIs
   *##########################################################################*/

  /// @note Our benchmark template requires this field.
  static constexpr auto
  EnumToString(        //
      const OPType e)  //
      -> std::string_view
  {
    switch (e) {
      case kMwCAS:
        return "MwCAS";
      default:
        throw std::runtime_error{"Found the unkown operation type."};
    }
  }

  /**
   * @brief Get the Operation Iter object
   *
   * @param thread_id A unique thread ID.
   * @param rand_seed A random seed.
   * @return An iterator for generating operations.
   * @note Our benchmark template requires this function.
   */
  [[nodiscard]]
  auto
  GetOPIter(  //
      [[maybe_unused]] const size_t thread_id,
      const size_t rand_seed) const  //
      -> OPIter
  {
    return OPIter{target_num_, arr_cap_, skew_parameter_, rand_seed, pos_index_};
  }

 private:
  /*##########################################################################*
   * Internal member variables
   *##########################################################################*/

  /// @brief The index for indicating actual positions in an array.
  std::vector<size_t> pos_index_{};

  /// @brief The capacity of an array.
  size_t arr_cap_{};

  /// @brief The number of target words for MwCAS.
  size_t target_num_{};

  /// @brief A skew parameter in Zipf's law.
  double skew_parameter_{};
};

}  // namespace dbgroup

#endif  // DBGROUP_MWCAS_BENCHMARK_OPERATION_ENGINE_HPP_
