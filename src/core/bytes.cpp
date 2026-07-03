#include "aegis/bytes.hpp"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace aegis {
namespace {
void require(std::size_t available, std::size_t wanted) {
  if (wanted > available) throw DecodeError("truncated binary field");
}
}
std::uint8_t ByteCursor::u8() { require(remaining(), 1); return bytes_[offset_++]; }
std::uint16_t ByteCursor::u16() {
  require(remaining(), 2); std::uint16_t v = static_cast<std::uint16_t>(bytes_[offset_]) |
      static_cast<std::uint16_t>(bytes_[offset_+1] << 8); offset_ += 2; return v;
}
std::uint32_t ByteCursor::u32() {
  require(remaining(), 4); std::uint32_t v = 0;
  for (int i=0;i<4;++i) v |= static_cast<std::uint32_t>(bytes_[offset_+i]) << (8*i);
  offset_ += 4; return v;
}
std::uint64_t ByteCursor::u64() {
  require(remaining(), 8); std::uint64_t v = 0;
  for (int i=0;i<8;++i) v |= static_cast<std::uint64_t>(bytes_[offset_+i]) << (8*i);
  offset_ += 8; return v;
}
std::vector<std::uint8_t> ByteCursor::blob(std::size_t count) {
  require(remaining(), count); std::vector<std::uint8_t> out(bytes_.begin()+static_cast<std::ptrdiff_t>(offset_), bytes_.begin()+static_cast<std::ptrdiff_t>(offset_+count)); offset_ += count; return out;
}
std::string ByteCursor::text(std::size_t count) { auto out = blob(count); return {out.begin(),out.end()}; }
std::string ByteCursor::short_text() { return text(u8()); }
ByteCursor ByteCursor::slice(std::size_t count) { require(remaining(), count); ByteCursor part(bytes_.subspan(offset_,count)); offset_ += count; return part; }
void ByteCursor::skip(std::size_t count) { require(remaining(),count); offset_ += count; }
void ByteWriter::u8(std::uint8_t value) { data_.push_back(value); }
void ByteWriter::u16(std::uint16_t value) { u8(static_cast<std::uint8_t>(value)); u8(static_cast<std::uint8_t>(value>>8)); }
void ByteWriter::u32(std::uint32_t value) { for(int i=0;i<4;++i) u8(static_cast<std::uint8_t>(value>>(8*i))); }
void ByteWriter::u64(std::uint64_t value) { for(int i=0;i<8;++i) u8(static_cast<std::uint8_t>(value>>(8*i))); }
void ByteWriter::blob(std::span<const std::uint8_t> value) { data_.insert(data_.end(),value.begin(),value.end()); }
void ByteWriter::text(std::string_view value) { data_.insert(data_.end(),value.begin(),value.end()); }
void ByteWriter::short_text(std::string_view value) { if(value.size()>255) throw DecodeError("text too long"); u8(static_cast<std::uint8_t>(value.size())); text(value); }
std::uint32_t crc32(std::span<const std::uint8_t> bytes) noexcept {
  std::uint32_t crc=0xffffffffu; for(auto b:bytes){ crc ^= b; for(int i=0;i<8;++i) crc=(crc>>1) ^ (0xedb88320u & (0u-(crc&1u))); } return ~crc;
}
std::uint64_t stable_hash(std::string_view text) noexcept { std::uint64_t v=1469598103934665603ULL; for(unsigned char c:text){v^=c;v*=1099511628211ULL;} return v; }
std::string hex(std::span<const std::uint8_t> bytes) { std::ostringstream s; s<<std::hex<<std::setfill('0'); for(auto b:bytes) s<<std::setw(2)<<static_cast<unsigned>(b); return s.str(); }
std::string normalize_text(std::string_view text) { std::string out; out.reserve(text.size()); for(unsigned char c:text) { if(std::isspace(c)) { if(!out.empty() && out.back()!=' ') out.push_back(' '); } else out.push_back(static_cast<char>(std::tolower(c))); } if(!out.empty()&&out.back()==' ') out.pop_back(); return out; }
bool is_text_safe(std::string_view text) noexcept { return std::all_of(text.begin(),text.end(),[](unsigned char c){return c>=0x20&&c<0x7f;}); }
}  // namespace aegis
