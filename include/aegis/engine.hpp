#pragma once
#include "aegis/capture.hpp"
#include "aegis/domain.hpp"
#include "aegis/recovery/checkpoint_journal.hpp"
#include "aegis/trace/fragment_trace.hpp"
#include "aegis/charging/charging_projection.hpp"
#include "aegis/replay/replay_book.hpp"
#include <span>
#include <string>
#include <vector>
namespace aegis {
struct EngineReport { std::size_t accepted{}; std::size_t rejected{}; std::size_t audits{}; std::string health; std::vector<std::string> observations; };
class Engine {
 public:
  EngineReport ingest(std::span<const std::uint8_t> capture);
  EngineReport replay(const Capture& capture);
  std::string health_report() const;
  const AuditJournal& audit() const noexcept { return audit_; }
 private:
  void apply(const Frame& frame, EngineReport& report);
  void register_subscriber(ByteCursor& c, EngineReport& report);
  void open_session(ByteCursor& c, EngineReport& report);
  void define_route(ByteCursor& c, EngineReport& report);
  void define_policy(ByteCursor& c, EngineReport& report);
  void stage_policy(ByteCursor& c, EngineReport& report);
  void commit_policy(ByteCursor& c, EngineReport& report);
  void quota_update(ByteCursor& c, EngineReport& report);
  void handover(ByteCursor& c, EngineReport& report);
  void extension(ByteCursor& c, EngineReport& report);
  void snapshot(ByteCursor& c, EngineReport& report);
  void rollback(ByteCursor& c, EngineReport& report);
  void heartbeat(ByteCursor& c, EngineReport& report);
  void close_session(ByteCursor& c, EngineReport& report);
  void fragment(ByteCursor& c, EngineReport& report);
  void diagnostics(ByteCursor& c, EngineReport& report);
  void checkpoint(ByteCursor& c, EngineReport& report);
  void journal_stage(ByteCursor& c, EngineReport& report);
  void checkpoint_rebuild(ByteCursor& c, EngineReport& report);
  void journal_finalize(ByteCursor& c, EngineReport& report);
  void trace_capture(ByteCursor& c, EngineReport& report);
  void fragment_coalesce(ByteCursor& c, EngineReport& report);
  void trace_report(ByteCursor& c, EngineReport& report);
  void charge_clause(ByteCursor& c, EngineReport& report);
  void charge_rewrite(ByteCursor& c, EngineReport& report);
  void charge_report(ByteCursor& c, EngineReport& report);
  void replay_token(ByteCursor& c, EngineReport& report);
  void replay_pin(ByteCursor& c, EngineReport& report);
  void replay_restore(ByteCursor& c, EngineReport& report);
  void replay_verify(ByteCursor& c, EngineReport& report);
  SubscriberTable subscribers_; RouteCache routes_; RouteIndex route_index_; PolicyStore policies_; PolicyCompiler compiler_; SessionStore sessions_; DispatchBook dispatch_; ExtensionStore extensions_; ExtensionPlanner extension_planner_; AuditJournal audit_; std::uint32_t epoch_ = 1; std::vector<std::string> fragments_;
  CheckpointBook checkpoints_; RecoveryJournal recovery_journal_; FragmentStore fragment_store_; TraceWindow trace_window_; ChargeStore charges_; ChargingPlanner charging_planner_; ReplayBook replay_book_; ReplayVerifier replay_verifier_;
};
}
