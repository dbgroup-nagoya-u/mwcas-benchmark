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
#include <cstddef>
#include <vector>

// external C++ libraries
#include <dbgroup/atomic/mwcas/stats.hpp>
#include <dbgroup/constants.hpp>

namespace dbgroup
{
/*############################################################################*
 * Competitor specific implementations: read operations
 *############################################################################*/

template <>
auto
MwCASTarget<DLFMwCAS>::Read(  //
    const size_t pos)         //
    -> size_t
{
  static_cast<void>(DLFMwCAS::Read<size_t>(&(target_fields_[pos].val), kRelaxed));
  return 1;
}

template <>
auto
MwCASTarget<CASN>::Read(  //
    const size_t pos)     //
    -> size_t
{
  [[maybe_unused]] const auto& guard = CASN::CreateEpochGuard();
  static_cast<void>(CASN::Read<size_t>(&(target_fields_[pos].val), kRelaxed));
  return 1;
}

template <>
auto
MwCASTarget<AOPT>::Read(  //
    const size_t pos)     //
    -> size_t
{
  [[maybe_unused]] const auto& guard = AOPT::CreateEpochGuard();
  static_cast<void>(AOPT::Read<size_t>(&(target_fields_[pos].val), kRelaxed));
  return 1;
}

template <>
auto
MwCASTarget<LFMwCAS>::Read(  //
    const size_t pos)        //
    -> size_t
{
  [[maybe_unused]] const auto& guard = LFMwCAS::CreateEpochGuard();
  static_cast<void>(LFMwCAS::Read<size_t>(&(target_fields_[pos].val), kRelaxed));
  return 1;
}

template <>
auto
MwCASTarget<LFMwCASLegacy>::Read(  //
    const size_t pos)              //
    -> size_t
{
  [[maybe_unused]] const auto& guard = LFMwCASLegacy::CreateEpochGuard();
  static_cast<void>(LFMwCASLegacy::Read<size_t>(&(target_fields_[pos].val), kRelaxed));
  return 1;
}

template <>
auto
MwCASTarget<LFMwCASWeak>::Read(  //
    const size_t pos)            //
    -> size_t
{
  [[maybe_unused]] const auto& guard = LFMwCASWeak::CreateEpochGuard();
  static_cast<void>(LFMwCASWeak::Read<size_t>(&(target_fields_[pos].val), kRelaxed));
  return 1;
}

template <>
auto
MwCASTarget<LFMwCASRC>::Read(  //
    const size_t pos)          //
    -> size_t
{
  // descriptors are protected by reference counts, so an epoch guard is not needed
  static_cast<void>(LFMwCASRC::Read<size_t>(&(target_fields_[pos].val), kRelaxed));
  return 1;
}

template <>
auto
MwCASTarget<OFMwCAS>::Read(  //
    const size_t pos)        //
    -> size_t
{
  [[maybe_unused]] const auto& guard = OFMwCAS::CreateEpochGuard();
  static_cast<void>(OFMwCAS::Read<size_t>(&(target_fields_[pos].val), kRelaxed));
  return 1;
}

template <>
auto
MwCASTarget<OFMwCASRC>::Read(  //
    const size_t pos)          //
    -> size_t
{
  // descriptors are protected by reference counts, so an epoch guard is not needed
  static_cast<void>(OFMwCASRC::Read<size_t>(&(target_fields_[pos].val), kRelaxed));
  return 1;
}

#ifdef MWCAS_BENCH_USE_PMWCAS
template <>
auto
MwCASTarget<PMwCAS>::Read(  //
    const size_t pos)       //
    -> size_t
{
  using PMwCASField = ::pmwcas::MwcTargetField<uint64_t>;

  auto* const epoch = pmwcas_desc_pool_->GetEpoch();
  epoch->Protect();
  auto* const addr = &(target_fields_[pos].val);
  static_cast<void>(std::bit_cast<PMwCASField*>(addr)->GetValueProtected());
  epoch->Unprotect();
  return 1;
}
#endif

/*############################################################################*
 * Competitor specific implementations: MwCAS operations
 *############################################################################*/

template <>
auto
MwCASTarget<DLFMwCAS>::MwCAS(    //
    const Operation& positions)  //
    -> size_t
{
  while (true) {
    MWCAS_STAT_INC(kMwCASAttempt);
    DLFMwCAS desc{};
    for (const auto pos : positions) {
      auto* const addr = &(target_fields_[pos].val);
      const auto old_val = DLFMwCAS::Read<size_t>(addr, kRelaxed);
      desc.AddMwCASTarget(addr, old_val, old_val + 1, kRelaxed);
    }
    if (desc.MwCAS()) break;
  }
  MWCAS_STAT_INC(kMwCASSuccess);
  return 1;
}

template <>
auto
MwCASTarget<CASN>::MwCAS(        //
    const Operation& positions)  //
    -> size_t
{
  while (true) {
    MWCAS_STAT_INC(kMwCASAttempt);
    [[maybe_unused]] const auto& guard = CASN::CreateEpochGuard();
    auto* const desc = CASN::GetDescriptor();
    for (const auto pos : positions) {
      auto* const addr = &(target_fields_[pos].val);
      const auto old_val = CASN::Read<size_t>(addr, kRelaxed);
      desc->AddMwCASTarget(addr, old_val, old_val + 1, kRelaxed);
    }
    if (desc->MwCAS()) break;
  }
  MWCAS_STAT_INC(kMwCASSuccess);
  return 1;
}

template <>
auto
MwCASTarget<AOPT>::MwCAS(        //
    const Operation& positions)  //
    -> size_t
{
  while (true) {
    MWCAS_STAT_INC(kMwCASAttempt);
    [[maybe_unused]] const auto& guard = AOPT::CreateEpochGuard();
    auto* const desc = AOPT::GetDescriptor();
    for (const auto pos : positions) {
      auto* const addr = &(target_fields_[pos].val);
      const auto old_val = AOPT::Read<size_t>(addr, kRelaxed);
      desc->AddMwCASTarget(addr, old_val, old_val + 1, kRelaxed);
    }
    if (desc->MwCAS()) break;
  }
  MWCAS_STAT_INC(kMwCASSuccess);
  return 1;
}

template <>
auto
MwCASTarget<LFMwCAS>::MwCAS(     //
    const Operation& positions)  //
    -> size_t
{
  while (true) {
    MWCAS_STAT_INC(kMwCASAttempt);
    [[maybe_unused]] const auto& guard = LFMwCAS::CreateEpochGuard();  // H11
    auto* const desc = LFMwCAS::GetDescriptor();
    for (const auto pos : positions) {
      auto* const addr = &(target_fields_[pos].val);
      const auto old_val = LFMwCAS::Read<size_t>(addr, kRelaxed);
      desc->AddMwCASTarget(addr, old_val, old_val + 1, kRelaxed);
    }
    if (desc->MwCAS()) break;
  }
  MWCAS_STAT_INC(kMwCASSuccess);
  return 1;
}

template <>
auto
MwCASTarget<LFMwCASLegacy>::MwCAS(  //
    const Operation& positions)     //
    -> size_t
{
  while (true) {
    MWCAS_STAT_INC(kMwCASAttempt);
    [[maybe_unused]] const auto& guard = LFMwCASLegacy::CreateEpochGuard();  // H11
    auto* const desc = LFMwCASLegacy::GetDescriptor();
    for (const auto pos : positions) {
      auto* const addr = &(target_fields_[pos].val);
      const auto old_val = LFMwCASLegacy::Read<size_t>(addr, kRelaxed);
      desc->AddMwCASTarget(addr, old_val, old_val + 1, kRelaxed);
    }
    if (desc->MwCAS()) break;
  }
  MWCAS_STAT_INC(kMwCASSuccess);
  return 1;
}

template <>
auto
MwCASTarget<LFMwCASWeak>::MwCAS(  //
    const Operation& positions)   //
    -> size_t
{
  while (true) {
    MWCAS_STAT_INC(kMwCASAttempt);
    [[maybe_unused]] const auto& guard = LFMwCASWeak::CreateEpochGuard();  // H11
    auto* const desc = LFMwCASWeak::GetDescriptor();
    for (const auto pos : positions) {
      auto* const addr = &(target_fields_[pos].val);
      const auto [old_val, word] = LFMwCASWeak::Read<size_t>(addr, kRelaxed);
      desc->AddMwCASTarget(addr, word, old_val + 1, kRelaxed);
    }
    if (desc->MwCAS()) break;
  }
  MWCAS_STAT_INC(kMwCASSuccess);
  return 1;
}

template <>
auto
MwCASTarget<LFMwCASRC>::MwCAS(   //
    const Operation& positions)  //
    -> size_t
{
  while (true) {  // descriptors are protected by reference counts
    MWCAS_STAT_INC(kMwCASAttempt);
    auto* const desc = LFMwCASRC::GetDescriptor();
    for (const auto pos : positions) {
      auto* const addr = &(target_fields_[pos].val);
      const auto old_val = LFMwCASRC::Read<size_t>(addr, kRelaxed);
      desc->AddMwCASTarget(addr, old_val, old_val + 1, kRelaxed);
    }
    if (desc->MwCAS()) break;
  }
  MWCAS_STAT_INC(kMwCASSuccess);
  return 1;
}

template <>
auto
MwCASTarget<OFMwCAS>::MwCAS(  //
    const Operation& positions) //
    -> size_t
{
  while (true) {
    MWCAS_STAT_INC(kMwCASAttempt);
    [[maybe_unused]] const auto& guard = OFMwCAS::CreateEpochGuard();
    auto* const desc = OFMwCAS::GetDescriptor();
    for (const auto pos : positions) {
      auto* const addr = &(target_fields_[pos].val);
      const auto old_val = OFMwCAS::Read<size_t>(addr, kRelaxed);
      desc->AddMwCASTarget(addr, old_val, old_val + 1, kRelaxed);
    }
    if (desc->MwCAS()) break;
  }
  MWCAS_STAT_INC(kMwCASSuccess);
  return 1;
}

template <>
auto
MwCASTarget<OFMwCASRC>::MwCAS(  //
    const Operation& positions) //
    -> size_t
{
  while (true) {  // descriptors are protected by reference counts
    MWCAS_STAT_INC(kMwCASAttempt);
    auto* const desc = OFMwCASRC::GetDescriptor();
    for (const auto pos : positions) {
      auto* const addr = &(target_fields_[pos].val);
      const auto old_val = OFMwCASRC::Read<size_t>(addr, kRelaxed);
      desc->AddMwCASTarget(addr, old_val, old_val + 1, kRelaxed);
    }
    if (desc->MwCAS()) break;
  }
  MWCAS_STAT_INC(kMwCASSuccess);
  return 1;
}

#ifdef MWCAS_BENCH_USE_PMWCAS
template <>
auto
MwCASTarget<PMwCAS>::MwCAS(      //
    const Operation& positions)  //
    -> size_t
{
  using PMwCASField = ::pmwcas::MwcTargetField<uint64_t>;

  while (true) {
    auto* const desc = pmwcas_desc_pool_->AllocateDescriptor();
    auto* const epoch = pmwcas_desc_pool_->GetEpoch();
    epoch->Protect();
    for (const auto pos : positions) {
      auto* const addr = &(target_fields_[pos].val);
      const auto old_val = std::bit_cast<PMwCASField*>(addr)->GetValueProtected();
      desc->AddEntry(addr, old_val, old_val + 1);
    }
    const auto success = desc->MwCAS();
    epoch->Unprotect();
    if (success) break;
  }
  return 1;
}
#endif

/*############################################################################*
 * Explicit instantiation definitions
 *############################################################################*/

template class MwCASTarget<DLFMwCAS>;
template class MwCASTarget<CASN>;
template class MwCASTarget<AOPT>;
template class MwCASTarget<LFMwCAS>;
template class MwCASTarget<LFMwCASLegacy>;
template class MwCASTarget<LFMwCASWeak>;
template class MwCASTarget<LFMwCASRC>;
template class MwCASTarget<OFMwCAS>;
template class MwCASTarget<OFMwCASRC>;
#ifdef MWCAS_BENCH_USE_PMWCAS
template class MwCASTarget<PMwCAS>;
#endif

}  // namespace dbgroup
