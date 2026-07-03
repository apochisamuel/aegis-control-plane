#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace aegis::components {

struct ErrorCatalogEntry {
  std::uint64_t key{};
  std::uint64_t value{};
  std::uint32_t epoch{};
  std::string label;
  bool active = true;
  std::uint32_t touches{};
};

struct ErrorCatalogSummary {
  std::size_t entries{};
  std::size_t active_entries{};
  std::uint64_t total{};
  std::uint64_t watermark{};
  std::uint64_t revisions{};
};

class ErrorCatalog {
 public:
  explicit ErrorCatalog(std::string scope = "error");

  void enroll(std::uint64_t key, std::uint64_t value, std::uint32_t epoch,
            std::string label);
  bool resolve(std::uint64_t key, std::uint64_t amount, std::uint32_t epoch);
  std::size_t group(std::uint32_t floor_epoch);

  bool deactivate(std::uint64_t key, std::string_view reason);
  std::optional<ErrorCatalogEntry> lookup(std::uint64_t key) const;
  std::vector<ErrorCatalogEntry> snapshot() const;
  std::vector<ErrorCatalogEntry> active_entries() const;
  std::map<std::uint32_t, std::uint64_t> totals_by_epoch() const;
  std::vector<std::string> anomalies(std::uint32_t expected_epoch) const;
  void merge(std::vector<ErrorCatalogEntry> incoming, std::uint32_t epoch);
  bool reconcile(std::uint64_t expected_total);

  [[nodiscard]] ErrorCatalogSummary summary() const;
  [[nodiscard]] std::string digest() const;
  [[nodiscard]] std::uint64_t total() const noexcept { return total_; }
  [[nodiscard]] std::uint64_t watermark() const noexcept { return watermark_; }

 private:
  void remeasure();
  std::string scope_;
  std::map<std::uint64_t, ErrorCatalogEntry> codes_;
  std::uint64_t total_{};
  std::uint64_t watermark_{};
  std::uint64_t revisions_{};
};

}  // namespace aegis::components
