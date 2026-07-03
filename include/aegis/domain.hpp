#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace aegis {
struct Subscriber { std::uint32_t id{}; std::uint32_t generation{}; std::string label; bool active=true; };
struct RouteRecord { std::uint32_t id{}; std::uint32_t generation{}; std::string destination; std::uint32_t weight{}; bool enabled=true; };
struct PolicyRule { std::uint32_t id{}; std::uint32_t revision{}; std::uint32_t route_id{}; std::uint32_t quota_limit{}; std::uint16_t priority{}; std::string label; };
struct Session { std::uint32_t id{}; std::uint32_t subscriber_id{}; std::uint32_t route_id{}; std::uint32_t policy_id{}; std::uint32_t generation{}; std::uint64_t usage{}; bool active=true; };
struct Extension { std::uint32_t session_id{}; std::uint8_t category{}; std::string value; };
struct AuditEvent { std::uint64_t ordinal{}; std::string source; std::string message; };

class SubscriberTable { public: void upsert(Subscriber value); std::optional<Subscriber> find(std::uint32_t id) const; std::vector<Subscriber> values() const; private: std::map<std::uint32_t,Subscriber> values_; };
class RouteCache { public: void upsert(RouteRecord value); RouteRecord* find(std::uint32_t id); const RouteRecord* find(std::uint32_t id) const; void compact(std::uint32_t minimum_generation); std::vector<RouteRecord> values() const; private: std::vector<std::unique_ptr<RouteRecord>> storage_; std::unordered_map<std::uint32_t,RouteRecord*> index_; };
struct RouteProjection { std::uint32_t id{}; std::uint32_t generation{}; std::string display; std::uint32_t score{}; };
class RouteIndex { public: void rebuild(const RouteCache& routes); const RouteProjection* select(std::uint32_t route_id) const; std::uint64_t revision() const noexcept { return revision_; } private: std::vector<std::unique_ptr<RouteProjection>> projections_; std::unordered_map<std::uint32_t,RouteProjection*> by_route_; std::uint64_t revision_{}; };
class PolicyStore { public: void stage(PolicyRule rule); std::optional<PolicyRule> staged(std::uint32_t id) const; std::optional<PolicyRule> activate(std::uint32_t id); std::optional<PolicyRule> active(std::uint32_t id) const; std::vector<PolicyRule> values() const; private: std::map<std::uint32_t,PolicyRule> staged_; std::map<std::uint32_t,PolicyRule> active_; };
struct CompiledPolicy { std::uint32_t policy_id{}; std::uint32_t revision{}; std::uint32_t route_id{}; std::string audit_route() const; private: friend class PolicyCompiler; const RouteProjection* projection_{}; std::string label_; };
class PolicyCompiler { public: std::shared_ptr<const CompiledPolicy> compile(const PolicyRule& rule, const RouteIndex& index); std::shared_ptr<const CompiledPolicy> find(std::uint32_t policy_id) const; private: std::map<std::uint32_t,std::shared_ptr<CompiledPolicy>> compiled_; };
class SessionStore { public: void upsert(Session value); Session* find(std::uint32_t id); const Session* find(std::uint32_t id) const; void erase(std::uint32_t id); std::vector<Session> values() const; private: std::map<std::uint32_t,Session> values_; };
class DispatchPlan { public: void compile(std::uint32_t session_id, std::shared_ptr<const CompiledPolicy> policy); std::string audit_label() const; std::uint32_t session_id() const noexcept {return session_id_;} std::uint32_t policy_revision() const noexcept{return policy_revision_;} private: std::uint32_t session_id_{}; std::uint32_t policy_revision_{}; std::shared_ptr<const CompiledPolicy> policy_; };
class DispatchBook { public: void put(DispatchPlan plan); DispatchPlan* find(std::uint32_t session_id); const DispatchPlan* find(std::uint32_t session_id) const; void erase(std::uint32_t session_id); std::vector<DispatchPlan> values() const; private: std::map<std::uint32_t,DispatchPlan> plans_; };
class ExtensionStore { public: void add(Extension extension); std::vector<Extension> for_session(std::uint32_t session_id) const; std::uint64_t revision(std::uint32_t session_id) const; private: std::multimap<std::uint32_t,Extension> values_; std::map<std::uint32_t,std::uint64_t> revisions_; };
struct ExtensionProjection { std::uint32_t session_id{}; std::uint32_t policy_revision{}; std::uint64_t extension_revision{}; std::size_t reserved_bytes{}; };
class ExtensionPlanner { public: std::shared_ptr<const ExtensionProjection> compile(std::uint32_t session_id, std::uint32_t policy_revision, const ExtensionStore& extensions); std::shared_ptr<const ExtensionProjection> find(std::uint32_t session_id) const; private: std::map<std::uint32_t,std::shared_ptr<ExtensionProjection>> projections_; };
class AuditJournal { public: void append(std::string source,std::string message); std::vector<AuditEvent> tail(std::size_t limit) const; std::size_t size() const noexcept{return entries_.size();} private: std::vector<AuditEvent> entries_; };
}
