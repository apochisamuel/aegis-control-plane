#include "aegis/charging/charging_projection.hpp"
#include <algorithm>
#include <vector>

namespace aegis {
namespace {
std::size_t output_width(const ChargeClause& clause) {
  std::size_t width = 4;
  for (auto byte : clause.payload) {
    if (clause.representation == ChargeRepresentation::binary) {
      ++width;
    } else if (byte < 0x20U || byte == static_cast<std::uint8_t>('\\') ||
               byte == static_cast<std::uint8_t>('"')) {
      width += 4;
    } else {
      ++width;
    }
  }
  return width;
}
}  // namespace

void ChargeStore::upsert(ChargeClause clause) {
  latest_[clause.session_id] = std::make_unique<ChargeClause>(std::move(clause));
}

ChargeClause* ChargeStore::latest(std::uint32_t session_id) {
  auto found = latest_.find(session_id);
  return found == latest_.end() ? nullptr : found->second.get();
}

const ChargeClause* ChargeStore::latest(std::uint32_t session_id) const {
  auto found = latest_.find(session_id);
  return found == latest_.end() ? nullptr : found->second.get();
}

bool ChargeStore::rewrite_from_recovery(std::uint32_t session_id, std::uint32_t policy_revision) {
  auto* clause = latest(session_id);
  if (clause == nullptr || clause->policy_revision != policy_revision ||
      clause->representation != ChargeRepresentation::binary) {
    return false;
  }
  clause->representation = ChargeRepresentation::text;
  return true;
}

std::shared_ptr<const ChargeProjection> ChargingPlanner::compile(std::uint32_t session_id,
                                                                  std::uint32_t policy_revision,
                                                                  const ChargeStore& store) {
  const auto* clause = store.latest(session_id);
  if (clause == nullptr || clause->policy_revision != policy_revision) {
    return {};
  }
  auto projection = std::make_shared<ChargeProjection>();
  projection->session_id = session_id;
  projection->policy_revision = policy_revision;
  projection->clause = clause;
  projection->reserved_bytes = output_width(*clause);
  projections_[session_id] = projection;
  return projection;
}

std::shared_ptr<const ChargeProjection> ChargingPlanner::find(std::uint32_t session_id) const {
  auto found = projections_.find(session_id);
  return found == projections_.end() ? std::shared_ptr<const ChargeProjection>{} : found->second;
}

std::string render_charge_projection(const ChargeProjection& projection) {
  if (projection.clause == nullptr) {
    return "charge:none";
  }
  std::vector<char> out(projection.reserved_bytes + 1);
  std::size_t at = 0;
  out[at++] = '[';
  out[at++] = projection.clause->representation == ChargeRepresentation::binary ? 'B' : 'T';
  out[at++] = ']';
  for (auto byte : projection.clause->payload) {
    if (projection.clause->representation == ChargeRepresentation::binary) {
      out[at++] = static_cast<char>(byte);
    } else if (byte < 0x20U || byte == static_cast<std::uint8_t>('\\') ||
               byte == static_cast<std::uint8_t>('"')) {
      static constexpr char digits[] = "0123456789abcdef";
      out[at++] = '\\';
      out[at++] = 'x';
      out[at++] = digits[byte >> 4U];
      out[at++] = digits[byte & 0x0FU];
    } else {
      out[at++] = static_cast<char>(byte);
    }
  }
  out[at++] = ';';
  return std::string(out.data(), at);
}

}  // namespace aegis
