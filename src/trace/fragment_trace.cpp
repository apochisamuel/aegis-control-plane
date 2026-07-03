#include "aegis/trace/fragment_trace.hpp"
#include <algorithm>
#include <sstream>
#include <utility>

namespace aegis {
namespace {
void rebuild_joined(FragmentGroup& group) {
  std::sort(group.segments.begin(), group.segments.end(), [](const FragmentSegment& left,
                                                              const FragmentSegment& right) {
    return left.ordinal < right.ordinal;
  });
  group.joined.clear();
  for (const auto& segment : group.segments) {
    group.joined += segment.bytes;
  }
}
}  // namespace

void FragmentStore::append(std::uint32_t group_id, std::uint32_t route_generation,
                           std::string bytes) {
  auto found = by_group_.find(group_id);
  if (found == by_group_.end()) {
    auto group = std::make_unique<FragmentGroup>();
    group->group_id = group_id;
    group->route_generation = route_generation;
    by_group_[group_id] = group.get();
    groups_.push_back(std::move(group));
    found = by_group_.find(group_id);
  }
  auto* group = found->second;
  group->route_generation = std::max(group->route_generation, route_generation);
  group->segments.push_back({static_cast<std::uint32_t>(group->segments.size()), std::move(bytes)});
  rebuild_joined(*group);
}

const FragmentGroup* FragmentStore::find(std::uint32_t group_id) const {
  auto found = by_group_.find(group_id);
  return found == by_group_.end() ? nullptr : found->second;
}

void FragmentStore::coalesce(std::uint32_t route_generation) {
  std::vector<std::unique_ptr<FragmentGroup>> rebuilt;
  rebuilt.reserve(groups_.size());
  for (const auto& group : groups_) {
    auto replacement = std::make_unique<FragmentGroup>(*group);
    replacement->route_generation = std::max(replacement->route_generation, route_generation);
    rebuild_joined(*replacement);
    rebuilt.push_back(std::move(replacement));
  }
  groups_.swap(rebuilt);
  by_group_.clear();
  for (const auto& group : groups_) {
    by_group_[group->group_id] = group.get();
  }
}

bool TraceWindow::capture(std::uint32_t session_id, std::uint32_t group_id,
                          std::uint32_t route_generation, const FragmentStore& store) {
  const auto* group = store.find(group_id);
  if (group == nullptr || group->segments.size() < 2) {
    return false;
  }
  views_.erase(std::remove_if(views_.begin(), views_.end(), [&](const TraceView& view) {
    return view.session_id == session_id && view.group_id == group_id;
  }), views_.end());
  views_.push_back({session_id, group_id, route_generation, group});
  return true;
}

std::string TraceWindow::render(std::uint32_t session_id, std::uint32_t group_id) const {
  auto found = std::find_if(views_.begin(), views_.end(), [&](const TraceView& view) {
    return view.session_id == session_id && view.group_id == group_id;
  });
  if (found == views_.end() || found->group == nullptr) {
    return "trace:none";
  }
  std::ostringstream out;
  out << "trace:" << found->session_id << ':' << found->route_generation << ':'
      << found->group->joined;
  return out.str();
}

}  // namespace aegis
