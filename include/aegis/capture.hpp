#pragma once
#include "aegis/bytes.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace aegis {

enum class FrameKind : std::uint8_t {
  register_subscriber=1, open_session=2, define_route=3, define_policy=4,
  stage_policy=5, commit_policy=6, quota_update=7, handover=8,
  extension=9, snapshot=10, rollback=11, heartbeat=12, close_session=13,
  fragment=14, audit=15, diagnostics=16,
  checkpoint=17, journal_stage=18, checkpoint_rebuild=19, journal_finalize=20,
  trace_capture=21, fragment_coalesce=22, trace_report=23,
  charge_clause=24, charge_rewrite=25, charge_report=26,
  replay_token=27, replay_pin=28, replay_restore=29, replay_verify=30
};
struct Frame { FrameKind kind{}; std::uint8_t flags{}; std::vector<std::uint8_t> payload; };
struct Capture { std::uint8_t version{}; std::vector<Frame> frames; };
Capture decode_capture(std::span<const std::uint8_t> input);
std::vector<std::uint8_t> encode_capture(const Capture& capture);
std::string frame_name(FrameKind kind);
}
