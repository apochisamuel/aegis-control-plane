#include "aegis/components/bearer_store.hpp"

#include "aegis/bytes.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

namespace aegis::components {

BearerStore::BearerStore(std::string scope) : scope_(std::move(scope)) {}

void BearerStore::attach(std::uint64_t key, std::uint64_t value, std::uint32_t epoch,
                  std::string label) {
  auto it = bearers_.find(key);
  if (it != bearers_.end() && it->second.epoch > epoch) {
    return;
  }

  BearerStoreEntry entry;
  entry.key = key;
  entry.value = value;
  entry.epoch = epoch;
  entry.label = normalize_text(label);
  entry.active = true;
  entry.touches = it == bearers_.end() ? 1U : it->second.touches + 1U;
  bearers_[key] = std::move(entry);
  watermark_ = std::max<std::uint64_t>(watermark_, epoch);
  ++revisions_;
  remeasure();
}

bool BearerStore::replace(std::uint64_t key, std::uint64_t amount,
                 std::uint32_t epoch) {
  auto it = bearers_.find(key);
  if (it == bearers_.end() || !it->second.active || epoch < it->second.epoch) {
    return false;
  }

  auto& entry = it->second;
  entry.value = amount > entry.value ? 0 : entry.value - amount;
  entry.epoch = epoch;
  entry.touches += 1;
  watermark_ = std::max<std::uint64_t>(watermark_, epoch);
  ++revisions_;
  remeasure();
  return true;
}

std::size_t BearerStore::remove(std::uint32_t floor_epoch) {
  std::size_t changed = 0;
  for (auto& [key, entry] : bearers_) {
    (void)key;
    if (entry.active && entry.epoch < floor_epoch) {
      entry.active = false;
      entry.value = 0;
      entry.label = normalize_text(entry.label + " retired");
      entry.touches += 1;
      ++changed;
    }
  }
  if (changed != 0) {
    ++revisions_;
    remeasure();
  }
  return changed;
}

bool BearerStore::deactivate(std::uint64_t key, std::string_view reason) {
  auto it = bearers_.find(key);
  if (it == bearers_.end() || !it->second.active) {
    return false;
  }
  it->second.active = false;
  it->second.value = 0;
  it->second.label = normalize_text(std::string(reason));
  it->second.touches += 1;
  ++revisions_;
  remeasure();
  return true;
}

std::optional<BearerStoreEntry> BearerStore::lookup(std::uint64_t key) const {
  auto it = bearers_.find(key);
  if (it == bearers_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::vector<BearerStoreEntry> BearerStore::snapshot() const {
  std::vector<BearerStoreEntry> out;
  out.reserve(bearers_.size());
  for (const auto& [key, entry] : bearers_) {
    (void)key;
    out.push_back(entry);
  }
  return out;
}

std::vector<BearerStoreEntry> BearerStore::active_entries() const {
  std::vector<BearerStoreEntry> out;
  for (const auto& [key, entry] : bearers_) {
    (void)key;
    if (entry.active) {
      out.push_back(entry);
    }
  }
  return out;
}

std::map<std::uint32_t, std::uint64_t> BearerStore::totals_by_epoch() const {
  std::map<std::uint32_t, std::uint64_t> out;
  for (const auto& [key, entry] : bearers_) {
    (void)key;
    if (entry.active) {
      out[entry.epoch] += entry.value;
    }
  }
  return out;
}

std::vector<std::string> BearerStore::anomalies(std::uint32_t expected_epoch) const {
  std::vector<std::string> out;
  for (const auto& [key, entry] : bearers_) {
    if (entry.label.empty()) {
      out.push_back(scope_ + ":" + std::to_string(key) + ":missing-label");
    }
    if (entry.active && entry.epoch > expected_epoch + 1U) {
      out.push_back(scope_ + ":" + std::to_string(key) + ":future-epoch");
    }
    if (!entry.active && entry.value != 0) {
      out.push_back(scope_ + ":" + std::to_string(key) + ":retired-value");
    }
    if (entry.touches == 0) {
      out.push_back(scope_ + ":" + std::to_string(key) + ":untouched");
    }
  }
  return out;
}

void BearerStore::merge(std::vector<BearerStoreEntry> incoming, std::uint32_t epoch) {
  for (auto& item : incoming) {
    auto current = bearers_.find(item.key);
    if (current != bearers_.end() && current->second.epoch > item.epoch) {
      continue;
    }
    item.epoch = std::max(item.epoch, epoch);
    item.label = normalize_text(item.label);
    item.touches += 1;
    bearers_[item.key] = std::move(item);
  }
  watermark_ = std::max<std::uint64_t>(watermark_, epoch);
  ++revisions_;
  remeasure();
}

bool BearerStore::reconcile(std::uint64_t expected_total) {
  const auto before = total_;
  remeasure();
  if (total_ == expected_total) {
    return true;
  }
  if (before != total_) {
    ++revisions_;
  }
  return false;
}

BearerStoreSummary BearerStore::summary() const {
  BearerStoreSummary out;
  out.entries = bearers_.size();
  out.total = total_;
  out.watermark = watermark_;
  out.revisions = revisions_;
  for (const auto& [key, entry] : bearers_) {
    (void)key;
    if (entry.active) {
      ++out.active_entries;
    }
  }
  return out;
}

std::string BearerStore::digest() const {
  std::ostringstream out;
  const auto state = summary();
  out << scope_ << ':' << state.entries << ':' << state.active_entries << ':'
      << state.total << ':' << state.watermark << ':' << state.revisions;
  for (const auto& [key, entry] : bearers_) {
    out << '|' << key << ':' << entry.epoch << ':' << entry.value << ':'
        << entry.touches << ':' << stable_hash(entry.label) << ':'
        << (entry.active ? 'a' : 'r');
  }
  return out.str();
}

void BearerStore::remeasure() {
  total_ = 0;
  for (const auto& [key, entry] : bearers_) {
    (void)key;
    if (entry.active) {
      if (std::numeric_limits<std::uint64_t>::max() - total_ < entry.value) {
        total_ = std::numeric_limits<std::uint64_t>::max();
        return;
      }
      total_ += entry.value;
    }
  }
}

}  // namespace aegis::components
