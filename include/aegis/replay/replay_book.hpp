#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace aegis {

struct ReplayToken {
  std::uint64_t sequence{};
  std::uint64_t nonce{};
  std::string label;
};

class ReplayBook {
 public:
  void insert(std::uint32_t session_id, ReplayToken token);
  const ReplayToken* find(std::uint32_t session_id, std::uint64_t sequence) const;
  void restore(std::uint64_t minimum_sequence);

 private:
  std::map<std::uint32_t, std::vector<ReplayToken>> buckets_;
};

struct ReplayCursor {
  std::uint32_t session_id{};
  std::uint64_t sequence{};
  const ReplayToken* token{};
};

class ReplayVerifier {
 public:
  bool pin(std::uint32_t session_id, std::uint64_t sequence, const ReplayBook& book);
  std::string verify(std::uint32_t session_id) const;

 private:
  std::map<std::uint32_t, ReplayCursor> cursors_;
};

}  // namespace aegis
