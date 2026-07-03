#include "aegis/aegis.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  if (size < 64 || size > 16*1024*1024) return 0;
  try { aegis::Engine engine; auto report=engine.ingest(std::span<const std::uint8_t>(data,size)); (void)report; } catch (...) {}
  return 0;
}
