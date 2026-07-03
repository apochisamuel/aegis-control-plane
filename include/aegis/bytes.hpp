#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace aegis {

class DecodeError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class ByteCursor {
 public:
  explicit ByteCursor(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}
  [[nodiscard]] std::size_t remaining() const noexcept { return bytes_.size() - offset_; }
  [[nodiscard]] std::size_t offset() const noexcept { return offset_; }
  [[nodiscard]] bool empty() const noexcept { return remaining() == 0; }
  std::uint8_t u8();
  std::uint16_t u16();
  std::uint32_t u32();
  std::uint64_t u64();
  std::vector<std::uint8_t> blob(std::size_t count);
  std::string text(std::size_t count);
  std::string short_text();
  ByteCursor slice(std::size_t count);
  void skip(std::size_t count);
 private:
  std::span<const std::uint8_t> bytes_;
  std::size_t offset_ = 0;
};

class ByteWriter {
 public:
  void u8(std::uint8_t value);
  void u16(std::uint16_t value);
  void u32(std::uint32_t value);
  void u64(std::uint64_t value);
  void blob(std::span<const std::uint8_t> value);
  void text(std::string_view value);
  void short_text(std::string_view value);
  [[nodiscard]] const std::vector<std::uint8_t>& data() const noexcept { return data_; }
  [[nodiscard]] std::vector<std::uint8_t> take() && { return std::move(data_); }
 private:
  std::vector<std::uint8_t> data_;
};

std::uint32_t crc32(std::span<const std::uint8_t> bytes) noexcept;
std::uint64_t stable_hash(std::string_view text) noexcept;
std::string hex(std::span<const std::uint8_t> bytes);
std::string normalize_text(std::string_view text);
bool is_text_safe(std::string_view text) noexcept;

}  // namespace aegis
