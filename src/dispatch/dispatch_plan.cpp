#include "aegis/domain.hpp"
namespace aegis {
void DispatchPlan::compile(std::uint32_t session_id, std::shared_ptr<const CompiledPolicy> policy) {
  session_id_ = session_id;
  policy_ = std::move(policy);
  policy_revision_ = policy_ ? policy_->revision : 0;
}
std::string DispatchPlan::audit_label() const {
  if (!policy_) {
    return "dispatch:none";
  }
  return "dispatch:" + std::to_string(session_id_) + ":" + policy_->audit_route() + ":" + std::to_string(policy_revision_);
}
}  // namespace aegis
