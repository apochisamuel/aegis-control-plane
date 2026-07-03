#include "aegis/components/bounded_writer.hpp"

#include "aegis/bytes.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

namespace aegis::components {

BoundedWriter::BoundedWriter(std::string scope) : scope_(std::move(scope)) {}

void BoundedWriter::open(std::uint64_t key, std::uint64_t value, std::uint32_t epoch,
                  std::string label) {
  auto it = spans_.find(key);
  if (it != spans_.end() && it->second.epoch > epoch) {
    return;
  }

  BoundedWriterEntry entry;
  entry.key = key;
  entry.value = value;
  entry.epoch = epoch;
  entry.label = normalize_text(label);
  entry.active = true;
  entry.touches = it == spans_.end() ? 1U : it->second.touches + 1U;
  spans_[key] = std::move(entry);
  watermark_ = std::max<std::uint64_t>(watermark_, epoch);
  ++revisions_;
  remeasure();
}

bool BoundedWriter::write(std::uint64_t key, std::uint64_t amount,
                 std::uint32_t epoch) {
  auto it = spans_.find(key);
  if (it == spans_.end() || !it->second.active || epoch < it->second.epoch) {
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

std::size_t BoundedWriter::finish(std::uint32_t floor_epoch) {
  std::size_t changed = 0;
  for (auto& [key, entry] : spans_) {
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

bool BoundedWriter::deactivate(std::uint64_t key, std::string_view reason) {
  auto it = spans_.find(key);
  if (it == spans_.end() || !it->second.active) {
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

std::optional<BoundedWriterEntry> BoundedWriter::lookup(std::uint64_t key) const {
  auto it = spans_.find(key);
  if (it == spans_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::vector<BoundedWriterEntry> BoundedWriter::snapshot() const {
  std::vector<BoundedWriterEntry> out;
  out.reserve(spans_.size());
  for (const auto& [key, entry] : spans_) {
    (void)key;
    out.push_back(entry);
  }
  return out;
}

std::vector<BoundedWriterEntry> BoundedWriter::active_entries() const {
  std::vector<BoundedWriterEntry> out;
  for (const auto& [key, entry] : spans_) {
    (void)key;
    if (entry.active) {
      out.push_back(entry);
    }
  }
  return out;
}

std::map<std::uint32_t, std::uint64_t> BoundedWriter::totals_by_epoch() const {
  std::map<std::uint32_t, std::uint64_t> out;
  for (const auto& [key, entry] : spans_) {
    (void)key;
    if (entry.active) {
      out[entry.epoch] += entry.value;
    }
  }
  return out;
}

std::vector<std::string> BoundedWriter::anomalies(std::uint32_t expected_epoch) const {
  std::vector<std::string> out;
  for (const auto& [key, entry] : spans_) {
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

void BoundedWriter::merge(std::vector<BoundedWriterEntry> incoming, std::uint32_t epoch) {
  for (auto& item : incoming) {
    auto current = spans_.find(item.key);
    if (current != spans_.end() && current->second.epoch > item.epoch) {
      continue;
    }
    item.epoch = std::max(item.epoch, epoch);
    item.label = normalize_text(item.label);
    item.touches += 1;
    spans_[item.key] = std::move(item);
  }
  watermark_ = std::max<std::uint64_t>(watermark_, epoch);
  ++revisions_;
  remeasure();
}

bool BoundedWriter::reconcile(std::uint64_t expected_total) {
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

BoundedWriterSummary BoundedWriter::summary() const {
  BoundedWriterSummary out;
  out.entries = spans_.size();
  out.total = total_;
  out.watermark = watermark_;
  out.revisions = revisions_;
  for (const auto& [key, entry] : spans_) {
    (void)key;
    if (entry.active) {
      ++out.active_entries;
    }
  }
  return out;
}

std::string BoundedWriter::digest() const {
  std::ostringstream out;
  const auto state = summary();
  out << scope_ << ':' << state.entries << ':' << state.active_entries << ':'
      << state.total << ':' << state.watermark << ':' << state.revisions;
  for (const auto& [key, entry] : spans_) {
    out << '|' << key << ':' << entry.epoch << ':' << entry.value << ':'
        << entry.touches << ':' << stable_hash(entry.label) << ':'
        << (entry.active ? 'a' : 'r');
  }
  return out.str();
}

void BoundedWriter::remeasure() {
  total_ = 0;
  for (const auto& [key, entry] : spans_) {
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
