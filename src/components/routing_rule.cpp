#include "aegis/components/routing_rule.hpp"

#include "aegis/bytes.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

namespace aegis::components {

RoutingRule::RoutingRule(std::string scope) : scope_(std::move(scope)) {}

void RoutingRule::install(std::uint64_t key, std::uint64_t value, std::uint32_t epoch,
                  std::string label) {
  auto it = rules_.find(key);
  if (it != rules_.end() && it->second.epoch > epoch) {
    return;
  }

  RoutingRuleEntry entry;
  entry.key = key;
  entry.value = value;
  entry.epoch = epoch;
  entry.label = normalize_text(label);
  entry.active = true;
  entry.touches = it == rules_.end() ? 1U : it->second.touches + 1U;
  rules_[key] = std::move(entry);
  watermark_ = std::max<std::uint64_t>(watermark_, epoch);
  ++revisions_;
  remeasure();
}

bool RoutingRule::rank(std::uint64_t key, std::uint64_t amount,
                 std::uint32_t epoch) {
  auto it = rules_.find(key);
  if (it == rules_.end() || !it->second.active || epoch < it->second.epoch) {
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

std::size_t RoutingRule::retire(std::uint32_t floor_epoch) {
  std::size_t changed = 0;
  for (auto& [key, entry] : rules_) {
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

bool RoutingRule::deactivate(std::uint64_t key, std::string_view reason) {
  auto it = rules_.find(key);
  if (it == rules_.end() || !it->second.active) {
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

std::optional<RoutingRuleEntry> RoutingRule::lookup(std::uint64_t key) const {
  auto it = rules_.find(key);
  if (it == rules_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::vector<RoutingRuleEntry> RoutingRule::snapshot() const {
  std::vector<RoutingRuleEntry> out;
  out.reserve(rules_.size());
  for (const auto& [key, entry] : rules_) {
    (void)key;
    out.push_back(entry);
  }
  return out;
}

std::vector<RoutingRuleEntry> RoutingRule::active_entries() const {
  std::vector<RoutingRuleEntry> out;
  for (const auto& [key, entry] : rules_) {
    (void)key;
    if (entry.active) {
      out.push_back(entry);
    }
  }
  return out;
}

std::map<std::uint32_t, std::uint64_t> RoutingRule::totals_by_epoch() const {
  std::map<std::uint32_t, std::uint64_t> out;
  for (const auto& [key, entry] : rules_) {
    (void)key;
    if (entry.active) {
      out[entry.epoch] += entry.value;
    }
  }
  return out;
}

std::vector<std::string> RoutingRule::anomalies(std::uint32_t expected_epoch) const {
  std::vector<std::string> out;
  for (const auto& [key, entry] : rules_) {
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

void RoutingRule::merge(std::vector<RoutingRuleEntry> incoming, std::uint32_t epoch) {
  for (auto& item : incoming) {
    auto current = rules_.find(item.key);
    if (current != rules_.end() && current->second.epoch > item.epoch) {
      continue;
    }
    item.epoch = std::max(item.epoch, epoch);
    item.label = normalize_text(item.label);
    item.touches += 1;
    rules_[item.key] = std::move(item);
  }
  watermark_ = std::max<std::uint64_t>(watermark_, epoch);
  ++revisions_;
  remeasure();
}

bool RoutingRule::reconcile(std::uint64_t expected_total) {
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

RoutingRuleSummary RoutingRule::summary() const {
  RoutingRuleSummary out;
  out.entries = rules_.size();
  out.total = total_;
  out.watermark = watermark_;
  out.revisions = revisions_;
  for (const auto& [key, entry] : rules_) {
    (void)key;
    if (entry.active) {
      ++out.active_entries;
    }
  }
  return out;
}

std::string RoutingRule::digest() const {
  std::ostringstream out;
  const auto state = summary();
  out << scope_ << ':' << state.entries << ':' << state.active_entries << ':'
      << state.total << ':' << state.watermark << ':' << state.revisions;
  for (const auto& [key, entry] : rules_) {
    out << '|' << key << ':' << entry.epoch << ':' << entry.value << ':'
        << entry.touches << ':' << stable_hash(entry.label) << ':'
        << (entry.active ? 'a' : 'r');
  }
  return out.str();
}

void RoutingRule::remeasure() {
  total_ = 0;
  for (const auto& [key, entry] : rules_) {
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
