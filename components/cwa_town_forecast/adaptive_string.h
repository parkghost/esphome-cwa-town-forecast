#pragma once

#include <string>
#include <cstring>

#ifdef ESP32
#include "esp_heap_caps.h"
#include "esp32-hal-psram.h"
#include "soc/soc_caps.h"
#endif

#include "esphome/core/helpers.h"

namespace esphome {
namespace cwa_town_forecast {

/**
 * @brief Adaptive string class that uses optimal memory allocation strategy
 * 
 * This class automatically selects the best memory allocation strategy based on
 * available memory types. On ESP32 with PSRAM, it uses external RAM when beneficial.
 * On systems without PSRAM or when PSRAM is not available, it falls back to
 * standard heap allocation, similar to RAMAllocator strategy.
 * 
 * Key features:
 * - Automatic PSRAM detection and usage
 * - Graceful fallback to internal RAM
 * - Compatible with std::string interface
 * - Memory-efficient for embedded systems
 */
class AdaptiveString {
private:
  char* data_;
  size_t size_;
  size_t capacity_;
  
  /**
   * @brief Allocate memory using the best available strategy
   * @param new_capacity Required capacity in bytes
   * 
   * Allocation priority:
   * 1. PSRAM (if available and beneficial for large allocations)
   * 2. Internal RAM (fallback or for small allocations)
   */
  void allocate_memory(size_t new_capacity) {
    if (new_capacity == 0) {
      deallocate_memory();
      return;
    }
    
    char* new_data = nullptr;
    
#ifdef ESP32
    // Use PSRAM for strings larger than minimum threshold to optimize memory usage
    static constexpr size_t PSRAM_ALLOCATION_THRESHOLD = 64;
    if (psramFound() && new_capacity > PSRAM_ALLOCATION_THRESHOLD) {
      new_data = static_cast<char*>(heap_caps_malloc(new_capacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
      if (new_data) {
        ESP_LOGVV("adaptive_string", "Allocated %zu bytes in PSRAM", new_capacity);
      }
    }
#endif
    
    // Fallback to internal RAM allocation (like RAMAllocator::NONE strategy)
    if (!new_data) {
#ifdef ESP32
      new_data = static_cast<char*>(heap_caps_malloc(new_capacity, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
#else
      new_data = static_cast<char*>(malloc(new_capacity));
#endif
      if (new_data) {
        ESP_LOGVV("adaptive_string", "Allocated %zu bytes in internal RAM", new_capacity);
      }
    }
    
    if (!new_data) {
      ESP_LOGE("adaptive_string", "Failed to allocate %zu bytes", new_capacity);
      return;
    }
    
    // Copy existing data if present
    if (data_ && size_ > 0 && new_capacity > size_) {
      memcpy(new_data, data_, size_);
    }
    
    deallocate_memory();
    data_ = new_data;
    capacity_ = new_capacity;
  }
  
  /**
   * @brief Deallocate memory regardless of allocation type
   */
  void deallocate_memory() {
    if (data_) {
#ifdef ESP32
      heap_caps_free(data_);  // Works for both PSRAM and internal RAM
#else
      free(data_);
#endif
      data_ = nullptr;
    }
    capacity_ = 0;
  }

public:
  /**
   * @brief Default constructor - creates empty string
   */
  AdaptiveString() : data_(nullptr), size_(0), capacity_(0) {}
  
  /**
   * @brief Construct from C-string
   * @param str Source C-string (null-terminated)
   */
  AdaptiveString(const char* str) : data_(nullptr), size_(0), capacity_(0) {
    if (str) {
      size_t len = strlen(str);
      allocate_memory(len + 1);
      if (data_) {
        memcpy(data_, str, len);
        data_[len] = '\0';
        size_ = len;
      }
    }
  }
  
  /**
   * @brief Construct from std::string
   * @param str Source std::string
   */
  AdaptiveString(const std::string& str) : AdaptiveString(str.c_str()) {}
  
  /**
   * @brief Copy constructor
   * @param other Source AdaptiveString
   */
  AdaptiveString(const AdaptiveString& other) : data_(nullptr), size_(0), capacity_(0) {
    *this = other;
  }
  
  /**
   * @brief Move constructor
   * @param other Source AdaptiveString (will be moved from)
   */
  AdaptiveString(AdaptiveString&& other) noexcept : 
    data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
  }
  
  /**
   * @brief Copy assignment operator
   * @param other Source AdaptiveString
   * @return Reference to this object
   */
  AdaptiveString& operator=(const AdaptiveString& other) {
    if (this != &other) {
      if (other.data_ && other.size_ > 0) {
        allocate_memory(other.size_ + 1);
        if (data_) {
          memcpy(data_, other.data_, other.size_);
          data_[other.size_] = '\0';
          size_ = other.size_;
        }
      } else {
        deallocate_memory();
        size_ = 0;
      }
    }
    return *this;
  }
  
  /**
   * @brief Move assignment operator
   * @param other Source AdaptiveString (will be moved from)
   * @return Reference to this object
   */
  AdaptiveString& operator=(AdaptiveString&& other) noexcept {
    if (this != &other) {
      deallocate_memory();
      data_ = other.data_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      other.data_ = nullptr;
      other.size_ = 0;
      other.capacity_ = 0;
    }
    return *this;
  }
  
  /**
   * @brief Assign from C-string
   * @param str Source C-string
   * @return Reference to this object
   */
  AdaptiveString& operator=(const char* str) {
    if (str) {
      size_t len = strlen(str);
      allocate_memory(len + 1);
      if (data_) {
        memcpy(data_, str, len);
        data_[len] = '\0';
        size_ = len;
      }
    } else {
      deallocate_memory();
      size_ = 0;
    }
    return *this;
  }
  
  /**
   * @brief Assign from std::string
   * @param str Source std::string
   * @return Reference to this object
   */
  AdaptiveString& operator=(const std::string& str) {
    return *this = str.c_str();
  }
  
  /**
   * @brief Destructor - automatically releases memory
   */
  ~AdaptiveString() {
    deallocate_memory();
  }
  
  /**
   * @brief Get C-string representation
   * @return Null-terminated C-string (never null)
   */
  const char* c_str() const {
    return data_ ? data_ : "";
  }
  
  /**
   * @brief Get string size in bytes (excluding null terminator)
   * @return String length
   */
  size_t size() const { return size_; }
  
  /**
   * @brief Get string length (alias for size())
   * @return String length
   */
  size_t length() const { return size_; }
  
  /**
   * @brief Check if string is empty
   * @return true if string has zero length
   */
  bool empty() const { return size_ == 0; }
  
  /**
   * @brief Convert to std::string
   * @return std::string copy of the data
   */
  std::string to_std_string() const {
    return data_ ? std::string(data_, size_) : std::string();
  }
  
  /**
   * @brief Implicit conversion to std::string
   * @return std::string copy of the data
   */
  operator std::string() const {
    return to_std_string();
  }
  
  /**
   * @brief Equality comparison with another AdaptiveString
   * @param other Other AdaptiveString
   * @return true if strings are equal
   */
  bool operator==(const AdaptiveString& other) const {
    if (size_ != other.size_) return false;
    if (size_ == 0) return true;
    return memcmp(data_, other.data_, size_) == 0;
  }
  
  /**
   * @brief Equality comparison with C-string
   * @param str C-string to compare with
   * @return true if strings are equal
   */
  bool operator==(const char* str) const {
    if (!str) return size_ == 0;
    return strcmp(c_str(), str) == 0;
  }
  
  /**
   * @brief Equality comparison with std::string
   * @param str std::string to compare with
   * @return true if strings are equal
   */
  bool operator==(const std::string& str) const {
    return to_std_string() == str;
  }
  
  /**
   * @brief Get allocated capacity in bytes
   * @return Current capacity
   */
  size_t capacity() const { return capacity_; }
  
  /**
   * @brief Check which memory type is being used
   * @return true if using PSRAM, false if using internal RAM
   */
  bool is_using_psram() const {
#ifdef ESP32
    if (!data_ || !psramFound()) return false;
    // Check if pointer is in PSRAM address range
    // ESP32-S3 PSRAM typically starts at 0x3C000000
    uintptr_t addr = reinterpret_cast<uintptr_t>(data_);
    return (addr >= 0x3C000000 && addr < 0x40000000);
#else
    return false;
#endif
  }
};

}  // namespace cwa_town_forecast
}  // namespace esphome