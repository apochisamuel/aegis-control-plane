#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace aegis::components {

struct PolicyRepositoryEntry {
  std::uint64_t key{};
  std::uint64_t value{};
  std::uint32_t epoch{};
  std::string label;
  bool active = true;
  std::uint32_t touches{};
};

struct PolicyRepositorySummary {
  std::size_t entries{};
  std::size_t active_entries{};
  std::uint64_t total{};
  std::uint64_t watermark{};
  std::uint64_t revisions{};
};

class PolicyRepository {
 public:
  explicit PolicyRepository(std::string scope = "policy_repo");

  void store(std::uint64_t key, std::uint64_t value, std::uint32_t epoch,
            std::string label);
  bool select(std::uint64_t key, std::uint64_t amount, std::uint32_t epoch);
  std::size_t prune(std::uint32_t floor_epoch);

  bool deactivate(std::uint64_t key, std::string_view reason);
  std::optional<PolicyRepositoryEntry> lookup(std::uint64_t key) const;
  std::vector<PolicyRepositoryEntry> snapshot() const;
  std::vector<PolicyRepositoryEntry> active_entries() const;
  std::map<std::uint32_t, std::uint64_t> totals_by_epoch() const;
  std::vector<std::string> anomalies(std::uint32_t expected_epoch) const;
  void merge(std::vector<PolicyRepositoryEntry> incoming, std::uint32_t epoch);
  bool reconcile(std::uint64_t expected_total);

  [[nodiscard]] PolicyRepositorySummary summary() const;
  [[nodiscard]] std::string digest() const;
  [[nodiscard]] std::uint64_t total() const noexcept { return total_; }
  [[nodiscard]] std::uint64_t watermark() const noexcept { return watermark_; }

 private:
  void remeasure();
  std::string scope_;
  std::map<std::uint64_t, PolicyRepositoryEntry> entries_;
  std::uint64_t total_{};
  std::uint64_t watermark_{};
  std::uint64_t revision_count_{};
};

}  // namespace aegis::components
