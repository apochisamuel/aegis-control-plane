#include "aegis/domain.hpp"
#include "aegis/bytes.hpp"
#include <algorithm>

namespace aegis {
void RouteIndex::rebuild(const RouteCache& routes) {
  std::vector<std::unique_ptr<RouteProjection>> rebuilt;
  const auto entries = routes.values();
  rebuilt.reserve(entries.size());
  for (const auto& route : entries) {
    auto projection = std::make_unique<RouteProjection>();
    projection->id = route.id;
    projection->generation = route.generation;
    projection->display = normalize_text(route.destination);
    projection->score = route.weight + static_cast<std::uint32_t>(projection->display.size());
    rebuilt.push_back(std::move(projection));
  }
  projections_.swap(rebuilt);
  by_route_.clear();
  for (const auto& projection : projections_) {
    by_route_[projection->id] = projection.get();
  }
  ++revision_;
}
const RouteProjection* RouteIndex::select(std::uint32_t route_id) const {
  auto found = by_route_.find(route_id);
  return found == by_route_.end() ? nullptr : found->second;
}
}  // namespace aegis
