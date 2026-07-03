#include "aegis/capture.hpp"
#include <array>
namespace aegis {
Capture decode_capture(std::span<const std::uint8_t> input) {
  ByteCursor c(input); if(c.remaining()<6) throw DecodeError("capture header missing");
  const auto a=c.u8(),g=c.u8(),c0=c.u8(),p=c.u8();
  if(a!='A'||g!='G'||c0!='C'||p!='P') throw DecodeError("capture magic mismatch");
  Capture out; out.version=c.u8(); c.u8(); if(out.version!=1) throw DecodeError("unsupported capture version");
  while(!c.empty()) { auto kind=static_cast<FrameKind>(c.u8()); auto flags=c.u8(); auto n=c.u16(); auto sub=c.slice(n); Frame f{kind,flags,sub.blob(sub.remaining())}; out.frames.push_back(std::move(f)); }
  return out;
}
std::vector<std::uint8_t> encode_capture(const Capture& capture) {
  ByteWriter w; w.text("AGCP"); w.u8(capture.version); w.u8(0);
  for(const auto& f:capture.frames){ if(f.payload.size()>65535) throw DecodeError("frame too long"); w.u8(static_cast<std::uint8_t>(f.kind));w.u8(f.flags);w.u16(static_cast<std::uint16_t>(f.payload.size()));w.blob(f.payload); }
  return std::move(w).take();
}
std::string frame_name(FrameKind k) { switch(k) {
 case FrameKind::register_subscriber:return "register_subscriber";case FrameKind::open_session:return "open_session";case FrameKind::define_route:return "define_route";case FrameKind::define_policy:return "define_policy";case FrameKind::stage_policy:return "stage_policy";case FrameKind::commit_policy:return "commit_policy";case FrameKind::quota_update:return "quota_update";case FrameKind::handover:return "handover";case FrameKind::extension:return "extension";case FrameKind::snapshot:return "snapshot";case FrameKind::rollback:return "rollback";case FrameKind::heartbeat:return "heartbeat";case FrameKind::close_session:return "close_session";case FrameKind::fragment:return "fragment";case FrameKind::audit:return "audit";case FrameKind::diagnostics:return "diagnostics";case FrameKind::checkpoint:return "checkpoint";case FrameKind::journal_stage:return "journal_stage";case FrameKind::checkpoint_rebuild:return "checkpoint_rebuild";case FrameKind::journal_finalize:return "journal_finalize";case FrameKind::trace_capture:return "trace_capture";case FrameKind::fragment_coalesce:return "fragment_coalesce";case FrameKind::trace_report:return "trace_report";case FrameKind::charge_clause:return "charge_clause";case FrameKind::charge_rewrite:return "charge_rewrite";case FrameKind::charge_report:return "charge_report";case FrameKind::replay_token:return "replay_token";case FrameKind::replay_pin:return "replay_pin";case FrameKind::replay_restore:return "replay_restore";case FrameKind::replay_verify:return "replay_verify"; } return "unknown"; }
}
