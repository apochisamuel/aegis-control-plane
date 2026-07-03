#include "aegis/aegis.hpp"
#include <cassert>
using namespace aegis;
int main(){ Capture c{1,{}}; ByteWriter a;a.u32(1);a.u32(1);a.short_text("user");c.frames.push_back({FrameKind::register_subscriber,0,a.data()});ByteWriter r;r.u32(2);r.u32(1);r.short_text("route");r.u32(1);c.frames.push_back({FrameKind::define_route,0,r.data()});ByteWriter s;s.u32(3);s.u32(1);s.u32(2);s.u32(1);c.frames.push_back({FrameKind::open_session,0,s.data()});Engine e;auto report=e.ingest(encode_capture(c));assert(report.accepted==3);assert(report.rejected==0);assert(report.health.find("sessions=1")!=std::string::npos);}
