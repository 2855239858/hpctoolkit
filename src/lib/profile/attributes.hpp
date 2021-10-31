// -*-Mode: C++;-*-

// * BeginRiceCopyright *****************************************************
//
// $HeadURL$
// $Id$
//
// --------------------------------------------------------------------------
// Part of HPCToolkit (hpctoolkit.org)
//
// Information about sources of support for research and development of
// HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
// --------------------------------------------------------------------------
//
// Copyright ((c)) 2019-2020, Rice University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage.
//
// ******************************************************* EndRiceCopyright *

#ifndef HPCTOOLKIT_PROFILE_ATTRIBUTES_H
#define HPCTOOLKIT_PROFILE_ATTRIBUTES_H

#include "accumulators.hpp"
<<<<<<< HEAD

#include "util/ragged_vector.hpp"
#include "util/ref_wrappers.hpp"
#include "lib/prof/pms-format.h"
#include "lib/prof-lean/id-tuple.h"

#include <functional>
#include <unordered_map>
#include "stdshim/filesystem.hpp"
#include <optional>
=======
#include "context-fwd.hpp"

#include "util/ragged_vector.hpp"
#include "util/ref_wrappers.hpp"
#include "util/streaming_sort.hpp"
#include "lib/prof/pms-format.h"
#include "lib/prof-lean/id-tuple.h"

#include "stdshim/filesystem.hpp"
#include <functional>
#include <optional>
#include <thread>
#include <unordered_map>
>>>>>>> 48ad9c63916bee835c74688f8291f586f5de1d78

namespace hpctoolkit {

class Context;
class SuperpositionedContext;
class CollaborativeContext;
class Metric;

/// Attributes unique to a particular thread within a profile. Whether this is
/// a thread (as in thd_create) or a process (as in an MPI process), this
/// roughly represents a single serial execution within the profile.
class ThreadAttributes {
public:
<<<<<<< HEAD
  /// Default (empty) constructor. No news is no news.
  ThreadAttributes();
  ThreadAttributes(const ThreadAttributes& o) = default;
  ~ThreadAttributes() = default;

  /// Get or set the process id of this Thread.
  // MT: Externally Synchronized
  const std::optional<unsigned long>& procid() const noexcept { return m_procid; }
  void procid(unsigned long);

  /// Get or set the number of timepoints emitted that are local to this Thread.
  // MT: Externally Synchronized
  const std::optional<unsigned long long>& timepointCnt() const noexcept { return m_timepointCnt; }
  void timepointCnt(unsigned long long);

  /// Get or set the hierarchical tuple assigned to this Thread. Should never
  /// be empty.
=======
  // Default (empty) constructor.
  ThreadAttributes();
  ~ThreadAttributes() = default;

  ThreadAttributes(const ThreadAttributes& o) = default;
  ThreadAttributes& operator=(const ThreadAttributes& o) = default;
  ThreadAttributes(ThreadAttributes&& o) = default;
  ThreadAttributes& operator=(ThreadAttributes&& o) = default;

  /// Check whether this ThreadAttributes is suitable for creating a Thread.
  // MT: Safe (const)
  bool ok() const noexcept;

  /// Get or set the hierarchical tuple assigned to this Thread.
  /// Cannot be set to an empty vector.
>>>>>>> 48ad9c63916bee835c74688f8291f586f5de1d78
  // MT: Externally Synchronized
  const std::vector<pms_id_t>& idTuple() const noexcept;
  void idTuple(std::vector<pms_id_t>);

<<<<<<< HEAD
private:
  // TODO: Remove these 4 fields and replace the bits above with functions that
  // scan m_idTuple for the relevant fields. Also make idTuples mandatory and
  // set once, probably during construction. All after the the other kind
  // constants are set up.
  // Then, later, remove those shims and just use idTuples moving forward.
  std::optional<unsigned long> m_procid;
  std::optional<unsigned long long> m_timepointCnt;
  mutable std::vector<pms_id_t> m_idTuple;
=======
  /// Get or set the vital timepoint statistics (maximum count and expected
  /// disorder) for this Thread.
  // MT: Externally Synchronized
  unsigned long long timepointMaxCount() const noexcept;
  void timepointStats(unsigned long long cnt, unsigned int disorder) noexcept;

private:
  std::vector<pms_id_t> m_idTuple;
  std::optional<std::pair<unsigned long long, unsigned int>> m_timepointStats;

  friend class ProfilePipeline;
  unsigned int timepointDisorder() const noexcept;

  class FinalizeState {
  public:
    FinalizeState();
    ~FinalizeState();

    FinalizeState(FinalizeState&&) = delete;
    FinalizeState& operator=(FinalizeState&&) = delete;
    FinalizeState(const FinalizeState&) = delete;
    FinalizeState& operator=(const FinalizeState&) = delete;

  private:
    friend class ThreadAttributes;

    class CountingLookupMap {
      std::mutex lock;
      std::unordered_map<uint64_t, uint64_t> map;
    public:
      CountingLookupMap() = default;
      ~CountingLookupMap() = default;

      uint64_t get(uint64_t);
    };
    struct LocalHash {
      std::hash<uint16_t> h_u16;
      std::size_t operator()(const std::vector<uint16_t>& v) const noexcept;
    };
    util::locked_unordered_map<uint16_t, CountingLookupMap> globalIdxMap;
    util::locked_unordered_map<std::vector<uint16_t>, CountingLookupMap,
                               std::shared_mutex, LocalHash> localIdxMap;

    std::thread server;
    std::mutex mpilock;
  };

  // Finalize this ThreadAttributes using the given shared state.
  void finalize(FinalizeState&);
>>>>>>> 48ad9c63916bee835c74688f8291f586f5de1d78
};

/// A single Thread within a Profile. Or something like that.
class Thread {
public:
  using ud_t = util::ragged_vector<const Thread&>;

<<<<<<< HEAD
  Thread(ud_t::struct_t& rs)
    : Thread(rs, ThreadAttributes()) {};
  Thread(ud_t::struct_t& rs, const ThreadAttributes& attr)
    : Thread(rs, ThreadAttributes(attr)) {};
  Thread(ud_t::struct_t& rs, ThreadAttributes&& attr)
    : userdata(rs, std::cref(*this)), attributes(std::move(attr)) {};
=======
  Thread(ud_t::struct_t& rs, ThreadAttributes attr);
>>>>>>> 48ad9c63916bee835c74688f8291f586f5de1d78
  Thread(Thread&& o)
    : userdata(std::move(o.userdata), std::cref(*this)),
      attributes(std::move(o.attributes)) {};

  mutable ud_t userdata;

<<<<<<< HEAD
  // Attributes accociated with this Thread.
=======
  /// Attributes accociated with this Thread.
>>>>>>> 48ad9c63916bee835c74688f8291f586f5de1d78
  ThreadAttributes attributes;

  /// Thread-local data that is temporary and should be removed quickly.
  class Temporary {
  public:
    // Access to the backing thread.
    operator Thread&() noexcept { return m_thread; }
    operator const Thread&() const noexcept { return m_thread; }
    Thread& thread() noexcept { return m_thread; }
    const Thread& thread() const noexcept { return m_thread; }

    // Movable, not copyable
    Temporary(const Temporary&) = delete;
    Temporary(Temporary&&) = default;

    /// Reference to the Metric data for a particular Context in this Thread.
    /// Returns `nullptr` if none is present.
    // MT: Safe (const), Unstable (before notifyThreadFinal)
    const auto* accumulatorsFor(const Context& c) const noexcept {
      return data.find(c);
    }

    /// Reference to all of the Metric data on Thread.
    // MT: Safe (const), Unstable (before notifyThreadFinal)
    const auto& accumulators() const noexcept { return data; }

  private:
    Thread& m_thread;

    friend class ProfilePipeline;
    Temporary(Thread& t) : m_thread(t) {};
    bool contributesToCollab = false;

<<<<<<< HEAD
=======
    // Bits needed for handling timepoints
    bool unboundedDisorder = false;
    util::bounded_streaming_sort_buffer<
      std::pair<ContextRef::const_t, std::chrono::nanoseconds>,
      util::compare_only_second<std::pair<ContextRef::const_t, std::chrono::nanoseconds>>
      > timepointSortBuf;
    std::vector<std::pair<ContextRef::const_t, std::chrono::nanoseconds>> timepointStaging;
    std::chrono::nanoseconds minTime = std::chrono::nanoseconds::max();
    std::chrono::nanoseconds maxTime = std::chrono::nanoseconds::min();

>>>>>>> 48ad9c63916bee835c74688f8291f586f5de1d78
    friend class Metric;
    util::locked_unordered_map<util::reference_index<const Context>,
      util::locked_unordered_map<util::reference_index<const Metric>,
        MetricAccumulator>> data;
    util::locked_unordered_map<util::reference_index<const SuperpositionedContext>,
      util::locked_unordered_map<util::reference_index<const Metric>,
        MetricAccumulator>> sp_data;
  };
};

/// Attributes unique to a particular profile. Since a profile represents an
/// entire single execution (potentially on multiple processors), all of these
/// should be constant within a profile. If it isn't, we just issue warnings.
class ProfileAttributes {
public:
  /// Default (empty) constructor. No news is no news.
  ProfileAttributes();
  ~ProfileAttributes() = default;

  /// Get or set the name of the program being executed. Usually this is the
  /// basename of the path, but just in case it isn't always...
  // MT: Externally Synchronized
  const std::optional<std::string>& name() const noexcept { return m_name; }
  void name(const std::string& s) { name(std::string(s)); };
  void name(std::string&&);

  /// Get or set the path to the program that was profiled.
  // MT: Externally Synchronized
  const std::optional<stdshim::filesystem::path>& path() const noexcept { return m_path; }
  void path(const stdshim::filesystem::path& p) {
    path(stdshim::filesystem::path(p));
  }
  void path(stdshim::filesystem::path&&);

  /// Get or set the job number corrosponding to the profiled program.
  /// If the job number is not known, has_job() will return false and job()
  /// will throw an error.
  // MT: Externally Synchronized
  const std::optional<unsigned long>& job() const noexcept { return m_job; }
  void job(unsigned long);

  /// Get or set individual environment variables. Note that the getter may
  /// return `nullptr` if the variable is not known to be set.
  // MT: Externally Synchronized
  const std::string* environment(const std::string& var) const noexcept;
  void environment(std::string var, std::string val);

  /// Access the entire environment for this profile.
  // MT: Unstable (const)
  const std::unordered_map<std::string, std::string>& environment() const noexcept {
    return m_env;
  }

  /// Append to the hierarchical tuple name dictionary
  // MT: Externally Synchronized
  void idtupleName(uint16_t kind, std::string name);

  /// Access the entire hierarchical tuple name dictionary
  // MT: Unstable (const)
  const std::unordered_map<uint16_t, std::string>& idtupleNames() const noexcept {
    return m_idtupleNames;
  }

  /// Merge another PAttr into this one. Uses the given callback to issue
  /// human-readable warnings (at least for now). Returns true if the process
  /// proceeded without errors.
  // MT: Externally Synchronized
  bool merge(const ProfileAttributes&);

private:
  std::optional<std::string> m_name;
  std::optional<unsigned long> m_job;
  std::optional<stdshim::filesystem::path> m_path;
  std::unordered_map<std::string, std::string> m_env;
  std::unordered_map<uint16_t, std::string> m_idtupleNames;
};

}

namespace std {
  std::ostream& operator<<(std::ostream&, const hpctoolkit::ThreadAttributes&) noexcept;
}

#endif  // HPCTOOLKIT_PROFILE_ATTRIBUTES_H
