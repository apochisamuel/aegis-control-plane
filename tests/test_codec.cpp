#include "aegis/aegis.hpp"
#include <cassert>
using namespace aegis;
int main(){ByteWriter w;w.u8(2);w.u16(513);w.u32(0x12345678);w.short_text("abc");ByteCursor c(w.data());assert(c.u8()==2);assert(c.u16()==513);assert(c.u32()==0x12345678);assert(c.short_text()=="abc");assert(c.empty());assert(normalize_text(" Hello\tWORLD ")=="hello world");}
