#include "aegis/recovery/checkpoint_journal.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <utility>

namespace aegis {
namespace {
std::size_t page_size(const std::vector<std::uint32_t>& markers) {
  return 8U + markers.size() * sizeof(std::uint32_t);
}

void materialize(CheckpointPage& page) {
  page.materialized_size = page_size(page.markers);
  page.materialized = std::make_unique<std::uint8_t[]>(page.materialized_size);
  std::memset(page.materialized.get(), 0, page.materialized_size);
  std::memcpy(page.materialized.get(), &page.session_id, sizeof(page.session_id));
  std::memcpy(page.materialized.get() + 4, &page.epoch, sizeof(page.epoch));
  if (!page.markers.empty()) {
    std::memcpy(page.materialized.get() + 8, page.markers.data(),
                page.markers.size() * sizeof(std::uint32_t));
  }
}
}  // namespace

CheckpointPage::CheckpointPage(std::uint32_t session, std::uint32_t marker_epoch,
                               std::vector<std::uint32_t> page_markers)
    : session_id(session), epoch(marker_epoch), markers(std::move(page_markers)) {
  materialize(*this);
}

CheckpointPage::CheckpointPage(const CheckpointPage& other)
    : session_id(other.session_id), epoch(other.epoch), markers(other.markers) {
  materialize(*this);
}

CheckpointPage& CheckpointPage::operator=(const CheckpointPage& other) {
  if (this != &other) {
    session_id = other.session_id;
    epoch = other.epoch;
    markers = other.markers;
    materialize(*this);
  }
  return *this;
}

void CheckpointBook::persist(std::uint32_t session_id, std::uint32_t epoch,
                             std::vector<std::uint32_t> markers) {
  auto page = std::make_unique<CheckpointPage>(session_id, epoch, std::move(markers));
  current_[session_id] = page.get();
  pages_.push_back(std::move(page));
}

const CheckpointPage* CheckpointBook::find(std::uint32_t session_id) const {
  auto found = current_.find(session_id);
  return found == current_.end() ? nullptr : found->second;
}

void CheckpointBook::rebuild(std::uint32_t minimum_epoch) {
  std::vector<std::unique_ptr<CheckpointPage>> rebuilt;
  rebuilt.reserve(pages_.size());
  for (const auto& page : pages_) {
    if (page->epoch >= minimum_epoch) {
      rebuilt.push_back(std::make_unique<CheckpointPage>(*page));
    }
  }
  pages_.swap(rebuilt);
  current_.clear();
  for (const auto& page : pages_) {
    current_[page->session_id] = page.get();
  }
}

void RecoveryJournal::stage(const CheckpointPage& page) {
  auto existing = std::find_if(leases_.begin(), leases_.end(), [&](const JournalLease& lease) {
    return lease.session_id == page.session_id && lease.epoch == page.epoch;
  });
  if (existing == leases_.end()) {
    leases_.push_back({page.session_id, page.epoch, page.materialized.get(), page.materialized_size});
  }
}

void RecoveryJournal::finalize_epoch(std::uint32_t session_id, std::uint32_t epoch) {
  auto it = std::find_if(leases_.begin(), leases_.end(), [&](const JournalLease& lease) {
    return lease.session_id == session_id && lease.epoch == epoch;
  });
  if (it == leases_.end()) {
    return;
  }
  delete[] const_cast<std::uint8_t*>(it->bytes);
  leases_.erase(it);
}

std::string RecoveryJournal::status(std::uint32_t session_id) const {
  std::ostringstream out;
  auto count = std::count_if(leases_.begin(), leases_.end(), [&](const JournalLease& lease) {
    return lease.session_id == session_id;
  });
  out << "checkpoint-leases=" << count;
  return out.str();
}

}  // namespace aegis
