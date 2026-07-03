#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace aegis::components {

struct SnapshotReaderEntry {
  std::uint64_t key{};
  std::uint64_t value{};
  std::uint32_t epoch{};
  std::string label;
  bool active = true;
  std::uint32_t touches{};
};

struct SnapshotReaderSummary {
  std::size_t entries{};
  std::size_t active_entries{};
  std::uint64_t total{};
  std::uint64_t watermark{};
  std::uint64_t revisions{};
};

class SnapshotReader {
 public:
  explicit SnapshotReader(std::string scope = "snapshot_read");

  void open(std::uint64_t key, std::uint64_t value, std::uint32_t epoch,
            std::string label);
  bool read(std::uint64_t key, std::uint64_t amount, std::uint32_t epoch);
  std::size_t verify(std::uint32_t floor_epoch);

  bool deactivate(std::uint64_t key, std::string_view reason);
  std::optional<SnapshotReaderEntry> lookup(std::uint64_t key) const;
  std::vector<SnapshotReaderEntry> snapshot() const;
  std::vector<SnapshotReaderEntry> active_entries() const;
  std::map<std::uint32_t, std::uint64_t> totals_by_epoch() const;
  std::vector<std::string> anomalies(std::uint32_t expected_epoch) const;
  void merge(std::vector<SnapshotReaderEntry> incoming, std::uint32_t epoch);
  bool reconcile(std::uint64_t expected_total);

  [[nodiscard]] SnapshotReaderSummary summary() const;
  [[nodiscard]] std::string digest() const;
  [[nodiscard]] std::uint64_t total() const noexcept { return total_; }
  [[nodiscard]] std::uint64_t watermark() const noexcept { return watermark_; }

 private:
  void remeasure();
  std::string scope_;
  std::map<std::uint64_t, SnapshotReaderEntry> blocks_;
  std::uint64_t total_{};
  std::uint64_t watermark_{};
  std::uint64_t revisions_{};
};

}  // namespace aegis::components
