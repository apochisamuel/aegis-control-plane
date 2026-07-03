#include "aegis/diagnostics.hpp"
#include <vector>
namespace aegis {
std::string render_extension_projection(const ExtensionStore& store, const ExtensionProjection& projection) {
  std::vector<char> out(projection.reserved_bytes+1);
  std::size_t at=0;
  for(const auto& extension:store.for_session(projection.session_id)) {
    out[at++]='[';
    out[at++]=static_cast<char>('0'+(extension.category%10));
    out[at++]=']';
    for(unsigned char c:extension.value) {
      if(c=='\\'||c=='\"') { out[at++]='\\';out[at++]=static_cast<char>(c); }
      else if(c<0x20) { static constexpr char alphabet[]="0123456789abcdef";out[at++]='\\';out[at++]='x';out[at++]=alphabet[c>>4];out[at++]=alphabet[c&15]; }
      else out[at++]=static_cast<char>(c);
    }
    out[at++]=';';
  }
  return std::string(out.data(),at);
}
}  // namespace aegis
