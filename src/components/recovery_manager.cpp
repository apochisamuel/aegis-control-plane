#include "aegis/components/recovery_manager.hpp"

#include "aegis/bytes.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

namespace aegis::components {

RecoveryManager::RecoveryManager(std::string scope) : scope_(std::move(scope)) {}

void RecoveryManager::plan(std::uint64_t key, std::uint64_t value, std::uint32_t epoch,
                  std::string label) {
  auto it = steps_.find(key);
  if (it != steps_.end() && it->second.epoch > epoch) {
    return;
  }

  RecoveryManagerEntry entry;
  entry.key = key;
  entry.value = value;
  entry.epoch = epoch;
  entry.label = normalize_text(label);
  entry.active = true;
  entry.touches = it == steps_.end() ? 1U : it->second.touches + 1U;
  steps_[key] = std::move(entry);
  watermark_ = std::max<std::uint64_t>(watermark_, epoch);
  ++revisions_;
  remeasure();
}

bool RecoveryManager::apply(std::uint64_t key, std::uint64_t amount,
                 std::uint32_t epoch) {
  auto it = steps_.find(key);
  if (it == steps_.end() || !it->second.active || epoch < it->second.epoch) {
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

std::size_t RecoveryManager::verify(std::uint32_t floor_epoch) {
  std::size_t changed = 0;
  for (auto& [key, entry] : steps_) {
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

bool RecoveryManager::deactivate(std::uint64_t key, std::string_view reason) {
  auto it = steps_.find(key);
  if (it == steps_.end() || !it->second.active) {
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

std::optional<RecoveryManagerEntry> RecoveryManager::lookup(std::uint64_t key) const {
  auto it = steps_.find(key);
  if (it == steps_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::vector<RecoveryManagerEntry> RecoveryManager::snapshot() const {
  std::vector<RecoveryManagerEntry> out;
  out.reserve(steps_.size());
  for (const auto& [key, entry] : steps_) {
    (void)key;
    out.push_back(entry);
  }
  return out;
}

std::vector<RecoveryManagerEntry> RecoveryManager::active_entries() const {
  std::vector<RecoveryManagerEntry> out;
  for (const auto& [key, entry] : steps_) {
    (void)key;
    if (entry.active) {
      out.push_back(entry);
    }
  }
  return out;
}

std::map<std::uint32_t, std::uint64_t> RecoveryManager::totals_by_epoch() const {
  std::map<std::uint32_t, std::uint64_t> out;
  for (const auto& [key, entry] : steps_) {
    (void)key;
    if (entry.active) {
      out[entry.epoch] += entry.value;
    }
  }
  return out;
}

std::vector<std::string> RecoveryManager::anomalies(std::uint32_t expected_epoch) const {
  std::vector<std::string> out;
  for (const auto& [key, entry] : steps_) {
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

void RecoveryManager::merge(std::vector<RecoveryManagerEntry> incoming, std::uint32_t epoch) {
  for (auto& item : incoming) {
    auto current = steps_.find(item.key);
    if (current != steps_.end() && current->second.epoch > item.epoch) {
      continue;
    }
    item.epoch = std::max(item.epoch, epoch);
    item.label = normalize_text(item.label);
    item.touches += 1;
    steps_[item.key] = std::move(item);
  }
  watermark_ = std::max<std::uint64_t>(watermark_, epoch);
  ++revisions_;
  remeasure();
}

bool RecoveryManager::reconcile(std::uint64_t expected_total) {
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

RecoveryManagerSummary RecoveryManager::summary() const {
  RecoveryManagerSummary out;
  out.entries = steps_.size();
  out.total = total_;
  out.watermark = watermark_;
  out.revisions = revisions_;
  for (const auto& [key, entry] : steps_) {
    (void)key;
    if (entry.active) {
      ++out.active_entries;
    }
  }
  return out;
}

std::string RecoveryManager::digest() const {
  std::ostringstream out;
  const auto state = summary();
  out << scope_ << ':' << state.entries << ':' << state.active_entries << ':'
      << state.total << ':' << state.watermark << ':' << state.revisions;
  for (const auto& [key, entry] : steps_) {
    out << '|' << key << ':' << entry.epoch << ':' << entry.value << ':'
        << entry.touches << ':' << stable_hash(entry.label) << ':'
        << (entry.active ? 'a' : 'r');
  }
  return out.str();
}

void RecoveryManager::remeasure() {
  total_ = 0;
  for (const auto& [key, entry] : steps_) {
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
