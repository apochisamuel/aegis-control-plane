#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace aegis {

enum class ChargeRepresentation : std::uint8_t { binary = 1, text = 2 };

struct ChargeClause {
  std::uint32_t session_id{};
  std::uint32_t policy_revision{};
  ChargeRepresentation representation{ChargeRepresentation::binary};
  std::vector<std::uint8_t> payload;
};

class ChargeStore {
 public:
  void upsert(ChargeClause clause);
  ChargeClause* latest(std::uint32_t session_id);
  const ChargeClause* latest(std::uint32_t session_id) const;
  bool rewrite_from_recovery(std::uint32_t session_id, std::uint32_t policy_revision);

 private:
  std::map<std::uint32_t, std::unique_ptr<ChargeClause>> latest_;
};

struct ChargeProjection {
  std::uint32_t session_id{};
  std::uint32_t policy_revision{};
  const ChargeClause* clause{};
  std::size_t reserved_bytes{};
};

class ChargingPlanner {
 public:
  std::shared_ptr<const ChargeProjection> compile(std::uint32_t session_id,
                                                   std::uint32_t policy_revision,
                                                   const ChargeStore& store);
  std::shared_ptr<const ChargeProjection> find(std::uint32_t session_id) const;

 private:
  std::map<std::uint32_t, std::shared_ptr<ChargeProjection>> projections_;
};

std::string render_charge_projection(const ChargeProjection& projection);

}  // namespace aegis
