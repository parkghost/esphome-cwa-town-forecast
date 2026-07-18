#pragma once

#include <cstddef>
#include <cstdlib>

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace cwa_town_forecast {

// Allocator for std containers: PSRAM-preferred with internal-RAM fallback
// (RAMAllocator's default flags). RAMAllocator returns nullptr on OOM but std
// containers require a valid pointer (exceptions are disabled on ESP-IDF), so
// allocate() fails loudly instead of letting the container write through
// nullptr and corrupt memory. Instances are interchangeable (deallocate() is
// flag-independent), hence the constant equality operators std containers
// require.
template<class T> struct PsramAllocator : public RAMAllocator<T> {
  PsramAllocator() = default;
  template<class U> PsramAllocator(const PsramAllocator<U> &other) : RAMAllocator<T>(other) {}
  template<class U> struct rebind { using other = PsramAllocator<U>; };

  T *allocate(size_t n) {
    T *ptr = RAMAllocator<T>::allocate(n);
    if (ptr == nullptr) {
      ESP_LOGE("cwa_town_forecast", "Out of memory allocating %zu bytes", n * sizeof(T));
      abort();
    }
    return ptr;
  }

  bool operator==(const PsramAllocator &) const { return true; }
  bool operator!=(const PsramAllocator &) const { return false; }
};

}  // namespace cwa_town_forecast
}  // namespace esphome
