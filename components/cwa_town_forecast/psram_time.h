#pragma once

#include <ctime>
#include <memory>

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32
#include "esp_memory_utils.h"
#endif

namespace esphome {
namespace cwa_town_forecast {

class PsramTime {
 private:
  std::tm *time_data_;
  // NONE flag = try PSRAM first, fall back to internal RAM
  RAMAllocator<std::tm> allocator_{RAMAllocator<std::tm>::NONE};

  std::tm *allocate_tm() {
    std::tm *ptr = allocator_.allocate(1);
    if (ptr) {
      memset(ptr, 0, sizeof(std::tm));
    } else {
      ESP_LOGE("psram_time", "Failed to allocate std::tm structure");
    }
    return ptr;
  }

  void deallocate_tm() {
    if (time_data_) {
      allocator_.deallocate(time_data_, 1);
      time_data_ = nullptr;
    }
  }

 public:
  PsramTime() : time_data_(nullptr) {}

  PsramTime(const std::tm &tm) : time_data_(nullptr) {
    time_data_ = allocate_tm();
    if (time_data_) {
      *time_data_ = tm;
    }
  }

  PsramTime(const PsramTime &other) : time_data_(nullptr) {
    if (other.time_data_) {
      time_data_ = allocate_tm();
      if (time_data_) {
        *time_data_ = *other.time_data_;
      }
    }
  }

  PsramTime(PsramTime &&other) noexcept : time_data_(other.time_data_) {
    other.time_data_ = nullptr;
  }

  PsramTime &operator=(const PsramTime &other) {
    if (this != &other) {
      if (other.time_data_) {
        if (!time_data_) {
          time_data_ = allocate_tm();
        }
        if (time_data_) {
          *time_data_ = *other.time_data_;
        }
      } else {
        deallocate_tm();
      }
    }
    return *this;
  }

  PsramTime &operator=(PsramTime &&other) noexcept {
    if (this != &other) {
      deallocate_tm();
      time_data_ = other.time_data_;
      other.time_data_ = nullptr;
    }
    return *this;
  }

  PsramTime &operator=(const std::tm &tm) {
    if (!time_data_) {
      time_data_ = allocate_tm();
    }
    if (time_data_) {
      *time_data_ = tm;
    }
    return *this;
  }

  ~PsramTime() { deallocate_tm(); }

  std::tm *get() { return time_data_; }
  const std::tm *get() const { return time_data_; }

  std::tm &operator*() {
    if (!time_data_) {
      time_data_ = allocate_tm();
    }
    return *time_data_;
  }

  const std::tm &operator*() const {
    static std::tm empty_tm = {};
    return time_data_ ? *time_data_ : empty_tm;
  }

  std::tm *operator->() {
    if (!time_data_) {
      time_data_ = allocate_tm();
    }
    return time_data_;
  }

  const std::tm *operator->() const { return time_data_; }

  bool is_valid() const { return time_data_ != nullptr; }

  std::tm to_tm() const { return time_data_ ? *time_data_ : std::tm{}; }

  operator std::tm() const { return to_tm(); }

  operator bool() const { return is_valid(); }

  void reset() { deallocate_tm(); }

  bool is_using_psram() const {
#ifdef USE_ESP32
    if (!time_data_)
      return false;
    return esp_ptr_external_ram(time_data_);
#else
    return false;
#endif
  }

  bool ensure_allocated() {
    if (!time_data_) {
      time_data_ = allocate_tm();
    }
    return time_data_ != nullptr;
  }
};

}  // namespace cwa_town_forecast
}  // namespace esphome
