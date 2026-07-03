#include "aegis/replay/replay_book.hpp"
#include <algorithm>
#include <sstream>
#include <utility>

namespace aegis {
void ReplayBook::insert(std::uint32_t session_id, ReplayToken token) {
  auto& bucket = buckets_[session_id];
  auto existing = std::find_if(bucket.begin(), bucket.end(), [&](const ReplayToken& value) {
    return value.sequence == token.sequence;
  });
  if (existing == bucket.end()) {
    bucket.push_back(std::move(token));
  } else {
    *existing = std::move(token);
  }
}

const ReplayToken* ReplayBook::find(std::uint32_t session_id, std::uint64_t sequence) const {
  auto bucket = buckets_.find(session_id);
  if (bucket == buckets_.end()) {
    return nullptr;
  }
  auto found = std::find_if(bucket->second.begin(), bucket->second.end(), [&](const ReplayToken& token) {
    return token.sequence == sequence;
  });
  return found == bucket->second.end() ? nullptr : &*found;
}

void ReplayBook::restore(std::uint64_t minimum_sequence) {
  std::map<std::uint32_t, std::vector<ReplayToken>> rebuilt;
  for (const auto& [session_id, bucket] : buckets_) {
    auto& target = rebuilt[session_id];
    for (const auto& token : bucket) {
      if (token.sequence >= minimum_sequence) {
        target.push_back(token);
      }
    }
    std::sort(target.begin(), target.end(), [](const ReplayToken& left, const ReplayToken& right) {
      if (left.sequence != right.sequence) {
        return left.sequence < right.sequence;
      }
      return left.nonce < right.nonce;
    });
  }
  buckets_.swap(rebuilt);
}

bool ReplayVerifier::pin(std::uint32_t session_id, std::uint64_t sequence, const ReplayBook& book) {
  const auto* token = book.find(session_id, sequence);
  if (token == nullptr) {
    return false;
  }
  cursors_[session_id] = {session_id, sequence, token};
  return true;
}

std::string ReplayVerifier::verify(std::uint32_t session_id) const {
  auto cursor = cursors_.find(session_id);
  if (cursor == cursors_.end() || cursor->second.token == nullptr) {
    return "replay:none";
  }
  std::ostringstream out;
  out << "replay:" << cursor->second.sequence << ':' << cursor->second.token->nonce << ':'
      << cursor->second.token->label;
  return out.str();
}

}  // namespace aegis
