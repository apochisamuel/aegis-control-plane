#include "aegis/domain.hpp"
#include "aegis/bytes.hpp"
#include <memory>

namespace aegis {
std::shared_ptr<const CompiledPolicy> PolicyCompiler::compile(const PolicyRule& rule, const RouteIndex& index) {
  const auto* projection = index.select(rule.route_id);
  if (projection == nullptr) {
    return {};
  }
  auto compiled = std::make_shared<CompiledPolicy>();
  compiled->policy_id = rule.id;
  compiled->revision = rule.revision;
  compiled->route_id = rule.route_id;
  compiled->projection_ = projection;
  compiled->label_ = normalize_text(rule.label);
  compiled_[rule.id] = compiled;
  return compiled;
}
std::shared_ptr<const CompiledPolicy> PolicyCompiler::find(std::uint32_t policy_id) const {
  auto found = compiled_.find(policy_id);
  return found == compiled_.end() ? std::shared_ptr<const CompiledPolicy>{} : found->second;
}
std::string CompiledPolicy::audit_route() const {
  if (projection_ == nullptr) {
    return "unbound";
  }
  return label_ + ":" + projection_->display + ":" + std::to_string(projection_->score);
}
}  // namespace aegis
