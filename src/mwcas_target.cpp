/*
 * Copyright 2024 Database Group, Nagoya University
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

// the corresponding header
#include "dbgroup/mwcas_benchmark/mwcas_target.hpp"

// C++ standard libraries
#include <atomic>
#include <cstddef>
#include <vector>

namespace
{
/*##############################################################################
 * Local constants
 *############################################################################*/

/// @brief An alias of the relaxed memory order.
constexpr auto kRelaxed = std::memory_order_relaxed;

}  // namespace

namespace dbgroup
{
/*##############################################################################
 * Competitor specific implementations
 *############################################################################*/

template <>
auto
MwCASTarget<DLFMwCAS>::Execute(  //
    [[maybe_unused]] const OPType type,
    const std::vector<size_t> &positions)  //
    -> size_t
{
  while (true) {
    DLFMwCAS desc{};
    for (const auto pos : positions) {
      auto *addr = &(target_fields_[pos].val);
      const auto old_val = DLFMwCAS::Read<size_t>(addr, kRelaxed);
      desc.AddMwCASTarget(addr, old_val, old_val + 1, kRelaxed);
    }
    if (desc.MwCAS()) break;
  }
  return 1;
}

template <>
auto
MwCASTarget<CASN>::Execute(  //
    [[maybe_unused]] const OPType type,
    const std::vector<size_t> &positions)  //
    -> size_t
{
  while (true) {
    [[maybe_unused]] const auto &guard = CASN::CreateEpochGuard();
    auto *desc = CASN::GetDescriptor();
    for (const auto pos : positions) {
      auto *addr = &(target_fields_[pos].val);
      const auto old_val = CASN::Read<size_t>(addr, kRelaxed);
      desc->AddMwCASTarget(addr, old_val, old_val + 1, kRelaxed);
    }
    if (desc->MwCAS()) break;
  }
  return 1;
}

template <>
auto
MwCASTarget<AOPT>::Execute(  //
    [[maybe_unused]] const OPType type,
    const std::vector<size_t> &positions)  //
    -> size_t
{
  while (true) {
    [[maybe_unused]] const auto &guard = AOPT::CreateEpochGuard();
    auto *desc = AOPT::GetDescriptor();
    for (const auto pos : positions) {
      auto *addr = &(target_fields_[pos].val);
      const auto old_val = AOPT::Read<size_t>(addr, kRelaxed);
      desc->AddMwCASTarget(addr, old_val, old_val + 1, kRelaxed);
    }
    if (desc->MwCAS()) break;
  }
  return 1;
}

template <>
auto
MwCASTarget<LFMwCAS>::Execute(  //
    [[maybe_unused]] const OPType type,
    const std::vector<size_t> &positions)  //
    -> size_t
{
  while (true) {
    auto *desc = LFMwCAS::GetDescriptor();
    for (const auto pos : positions) {
      auto *addr = &(target_fields_[pos].val);
      const auto [old_val, word] = LFMwCAS::Read<size_t>(addr, kRelaxed);
      desc->AddMwCASTarget(addr, word, old_val + 1, kRelaxed);
    }
    if (desc->MwCAS()) break;
  }
  return 1;
}

#ifdef MWCAS_BENCH_USE_PMWCAS
template <>
auto
MwCASTarget<PMwCAS>::Execute(  //
    [[maybe_unused]] const OPType type,
    const std::vector<size_t> &positions)  //
    -> size_t
{
  using PMwCASField = ::pmwcas::MwcTargetField<uint64_t>;

  while (true) {
    auto *desc = pmwcas_desc_pool_->AllocateDescriptor();
    auto *epoch = pmwcas_desc_pool_->GetEpoch();
    epoch->Protect();
    for (const auto pos : positions) {
      auto *addr = &(target_fields_[pos].val);
      const auto old_val = std::bit_cast<PMwCASField *>(addr)->GetValueProtected();
      desc->AddEntry(addr, old_val, old_val + 1);
    }
    const auto success = desc->MwCAS();
    epoch->Unprotect();
    if (success) break;
  }
  return 1;
}
#endif

/*##############################################################################
 * Explicit instantiation definitions
 *############################################################################*/

template class MwCASTarget<DLFMwCAS>;
template class MwCASTarget<CASN>;
template class MwCASTarget<AOPT>;
template class MwCASTarget<LFMwCAS>;
#ifdef MWCAS_BENCH_USE_PMWCAS
template class MwCASTarget<PMwCAS>;
#endif

}  // namespace dbgroup
