#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "esphome/core/log.h"

#include "psram_allocator.h"

namespace esphome {
namespace cwa_town_forecast {

// Deduplicating storage for forecast element values: unique NUL-terminated
// strings are appended to one PSRAM-preferred buffer and referenced by 16-bit
// offsets. Real CWA responses carry only ~100 unique values (a few KB) across
// hundreds of slots, so one growing allocation replaces hundreds of per-value
// strings and the per-slot cost shrinks to a {key, offset} pair.
//
// Usage rules:
// - Pointers returned by get() are invalidated by the next intern() (the
//   buffer may reallocate); copy the value out before interning again.
// - Never pass a pointer obtained from get() back into intern(): inserting a
//   range that aliases the pool's own buffer is undefined behavior.
class StringPool {
 public:
  // Offset 0 is always the empty string; it doubles as the overflow fallback.
  StringPool() {
    data_.reserve(1024);
    data_.push_back('\0');
  }

  const char *get(uint16_t offset) const { return offset < data_.size() ? data_.data() + offset : ""; }

  // Returns the offset of str, appending it if not seen before.
  uint16_t intern(const char *str) {
    if (str == nullptr || str[0] == '\0')
      return 0;
    const size_t len = strlen(str);
    size_t off = 0;
    while (off < data_.size()) {
      const char *entry = data_.data() + off;
      const size_t entry_len = strlen(entry);
      if (entry_len == len && memcmp(entry, str, len) == 0)
        return static_cast<uint16_t>(off);
      off += entry_len + 1;
    }
    if (data_.size() + len + 1 > MAX_SIZE) {
      ESP_LOGW("cwa_town_forecast", "String pool full (%zu bytes), dropping value: %s", data_.size(), str);
      return 0;
    }
    const uint16_t new_off = static_cast<uint16_t>(data_.size());
    data_.insert(data_.end(), str, str + len + 1);
    return new_off;
  }

  size_t size() const { return data_.size(); }

 private:
  static constexpr size_t MAX_SIZE = 0xFFFF;
  std::vector<char, PsramAllocator<char>> data_;
};

}  // namespace cwa_town_forecast
}  // namespace esphome
