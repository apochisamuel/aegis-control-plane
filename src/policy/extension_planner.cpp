#include "aegis/domain.hpp"
#include <memory>
namespace aegis {
namespace {
std::size_t rendered_width(const Extension& extension) {
  std::size_t width = 4;
  for (unsigned char value : extension.value) {
    if (value == '\\' || value == '"') {
      width += 2;
    } else if (value < 0x20U) {
      width += 4;
    } else {
      ++width;
    }
  }
  return width;
}
}  // namespace
std::shared_ptr<const ExtensionProjection> ExtensionPlanner::compile(std::uint32_t session_id, std::uint32_t policy_revision, const ExtensionStore& extensions) {
  auto projection=std::make_shared<ExtensionProjection>();
  projection->session_id=session_id;
  projection->policy_revision=policy_revision;
  projection->extension_revision=extensions.revision(session_id);
  for(const auto& extension:extensions.for_session(session_id)) projection->reserved_bytes += rendered_width(extension);
  projections_[session_id]=projection;
  return projection;
}
std::shared_ptr<const ExtensionProjection> ExtensionPlanner::find(std::uint32_t session_id) const {
  auto found=projections_.find(session_id);
  return found==projections_.end()?std::shared_ptr<const ExtensionProjection>{}:found->second;
}
}  // namespace aegis
