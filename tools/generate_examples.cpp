#include "aegis/aegis.hpp"
#include <fstream>
#include <iostream>
using namespace aegis;
static Frame make(FrameKind k, const ByteWriter& w){return {k,0,w.data()};}
int main(int argc,char**argv){if(argc!=2)return 2;Capture c{1,{}};ByteWriter a;a.u32(7);a.u32(1);a.short_text("demo");c.frames.push_back(make(FrameKind::register_subscriber,a));ByteWriter r;r.u32(20);r.u32(2);r.short_text("core.example");r.u32(10);c.frames.push_back(make(FrameKind::define_route,r));ByteWriter s;s.u32(11);s.u32(7);s.u32(20);s.u32(2);c.frames.push_back(make(FrameKind::open_session,s));ByteWriter p;p.u32(31);p.u32(1);p.u32(20);p.u32(100);p.u16(3);p.short_text("default");c.frames.push_back(make(FrameKind::define_policy,p));ByteWriter st;st.u32(11);st.u32(31);c.frames.push_back(make(FrameKind::stage_policy,st));ByteWriter cm;cm.u32(31);c.frames.push_back(make(FrameKind::commit_policy,cm));auto data=encode_capture(c);std::ofstream out(argv[1],std::ios::binary);out.write(reinterpret_cast<const char*>(data.data()),static_cast<std::streamsize>(data.size()));}
