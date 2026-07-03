#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace aegis {

struct FragmentSegment {
  std::uint32_t ordinal{};
  std::string bytes;
};

struct FragmentGroup {
  std::uint32_t group_id{};
  std::uint32_t route_generation{};
  std::vector<FragmentSegment> segments;
  std::string joined;
};

class FragmentStore {
 public:
  void append(std::uint32_t group_id, std::uint32_t route_generation, std::string bytes);
  const FragmentGroup* find(std::uint32_t group_id) const;
  void coalesce(std::uint32_t route_generation);

 private:
  std::vector<std::unique_ptr<FragmentGroup>> groups_;
  std::map<std::uint32_t, FragmentGroup*> by_group_;
};

struct TraceView {
  std::uint32_t session_id{};
  std::uint32_t group_id{};
  std::uint32_t route_generation{};
  const FragmentGroup* group{};
};

class TraceWindow {
 public:
  bool capture(std::uint32_t session_id, std::uint32_t group_id,
               std::uint32_t route_generation, const FragmentStore& store);
  std::string render(std::uint32_t session_id, std::uint32_t group_id) const;

 private:
  std::vector<TraceView> views_;
};

}  // namespace aegis
