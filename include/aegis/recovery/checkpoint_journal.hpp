#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace aegis {

struct CheckpointPage {
  std::uint32_t session_id{};
  std::uint32_t epoch{};
  std::vector<std::uint32_t> markers;
  std::unique_ptr<std::uint8_t[]> materialized;
  std::size_t materialized_size{};

  CheckpointPage() = default;
  CheckpointPage(std::uint32_t session, std::uint32_t marker_epoch,
                 std::vector<std::uint32_t> page_markers);
  CheckpointPage(const CheckpointPage& other);
  CheckpointPage& operator=(const CheckpointPage& other);
  CheckpointPage(CheckpointPage&&) noexcept = default;
  CheckpointPage& operator=(CheckpointPage&&) noexcept = default;
};

class CheckpointBook {
 public:
  void persist(std::uint32_t session_id, std::uint32_t epoch,
               std::vector<std::uint32_t> markers);
  const CheckpointPage* find(std::uint32_t session_id) const;
  void rebuild(std::uint32_t minimum_epoch);
  std::size_t size() const noexcept { return pages_.size(); }

 private:
  std::vector<std::unique_ptr<CheckpointPage>> pages_;
  std::map<std::uint32_t, CheckpointPage*> current_;
};

struct JournalLease {
  std::uint32_t session_id{};
  std::uint32_t epoch{};
  const std::uint8_t* bytes{};
  std::size_t size{};
};

class RecoveryJournal {
 public:
  void stage(const CheckpointPage& page);
  void finalize_epoch(std::uint32_t session_id, std::uint32_t epoch);
  std::string status(std::uint32_t session_id) const;

 private:
  std::vector<JournalLease> leases_;
};

}  // namespace aegis
