#pragma once

#include <ctime>
#include <memory>

#ifdef ESP32
#include "esp_heap_caps.h"
#include "esp32-hal-psram.h"
#include "soc/soc_caps.h"
#endif

#include "esphome/core/helpers.h"

namespace esphome {
namespace cwa_town_forecast {

/**
 * @brief Adaptive time structure that uses optimal memory allocation strategy
 * 
 * This class automatically selects the best memory allocation strategy for storing
 * time data (std::tm structure). On ESP32 with PSRAM, it uses external RAM when
 * beneficial. On systems without PSRAM or when not available, it falls back to
 * standard heap allocation, similar to RAMAllocator strategy.
 * 
 * Key features:
 * - Automatic PSRAM detection and usage
 * - Graceful fallback to internal RAM
 * - Compatible with std::tm interface
 * - Memory-efficient for embedded systems
 * - Lazy allocation (only allocates when needed)
 */
class AdaptiveTime {
private:
  std::tm* time_data_;
  
  /**
   * @brief Allocate std::tm structure using the best available strategy
   * @return Pointer to allocated std::tm or nullptr on failure
   * 
   * Allocation priority:
   * 1. PSRAM (if available)
   * 2. Internal RAM (fallback)
   */
  std::tm* allocate_tm() {
    std::tm* ptr = nullptr;
    
#ifdef ESP32
    // Try PSRAM allocation first when available
    if (psramFound()) {
      ptr = static_cast<std::tm*>(heap_caps_malloc(sizeof(std::tm), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
      if (ptr) {
        ESP_LOGVV("adaptive_time", "Allocated std::tm in PSRAM");
      }
    }
    
    // Fallback to internal RAM allocation (like RAMAllocator::NONE strategy)
    if (!ptr) {
      ptr = static_cast<std::tm*>(heap_caps_malloc(sizeof(std::tm), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
      if (ptr) {
        ESP_LOGVV("adaptive_time", "Allocated std::tm in internal RAM");
      }
    }
#else
    // Standard allocation for non-ESP32 platforms
    ptr = static_cast<std::tm*>(malloc(sizeof(std::tm)));
#endif
    
    if (ptr) {
      memset(ptr, 0, sizeof(std::tm));  // Initialize to zero
    } else {
      ESP_LOGE("adaptive_time", "Failed to allocate std::tm structure");
    }
    
    return ptr;
  }
  
  /**
   * @brief Deallocate time data regardless of allocation type
   */
  void deallocate_tm() {
    if (time_data_) {
#ifdef ESP32
      heap_caps_free(time_data_);  // Works for both PSRAM and internal RAM
#else
      free(time_data_);
#endif
      time_data_ = nullptr;
    }
  }

public:
  /**
   * @brief Default constructor - creates uninitialized time
   * Note: Time data is allocated lazily when first accessed
   */
  AdaptiveTime() : time_data_(nullptr) {}
  
  /**
   * @brief Construct from std::tm structure
   * @param tm Source time structure
   */
  AdaptiveTime(const std::tm& tm) : time_data_(nullptr) {
    time_data_ = allocate_tm();
    if (time_data_) {
      *time_data_ = tm;
    }
  }
  
  /**
   * @brief Copy constructor
   * @param other Source AdaptiveTime
   */
  AdaptiveTime(const AdaptiveTime& other) : time_data_(nullptr) {
    if (other.time_data_) {
      time_data_ = allocate_tm();
      if (time_data_) {
        *time_data_ = *other.time_data_;
      }
    }
  }
  
  /**
   * @brief Move constructor
   * @param other Source AdaptiveTime (will be moved from)
   */
  AdaptiveTime(AdaptiveTime&& other) noexcept : time_data_(other.time_data_) {
    other.time_data_ = nullptr;
  }
  
  /**
   * @brief Copy assignment operator
   * @param other Source AdaptiveTime
   * @return Reference to this object
   */
  AdaptiveTime& operator=(const AdaptiveTime& other) {
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
  
  /**
   * @brief Move assignment operator
   * @param other Source AdaptiveTime (will be moved from)
   * @return Reference to this object
   */
  AdaptiveTime& operator=(AdaptiveTime&& other) noexcept {
    if (this != &other) {
      deallocate_tm();
      time_data_ = other.time_data_;
      other.time_data_ = nullptr;
    }
    return *this;
  }
  
  /**
   * @brief Assign from std::tm structure
   * @param tm Source time structure
   * @return Reference to this object
   */
  AdaptiveTime& operator=(const std::tm& tm) {
    if (!time_data_) {
      time_data_ = allocate_tm();
    }
    if (time_data_) {
      *time_data_ = tm;
    }
    return *this;
  }
  
  /**
   * @brief Destructor - automatically releases memory
   */
  ~AdaptiveTime() {
    deallocate_tm();
  }
  
  /**
   * @brief Get raw pointer to std::tm (mutable)
   * @return Pointer to std::tm or nullptr if not allocated
   */
  std::tm* get() { 
    return time_data_; 
  }
  
  /**
   * @brief Get raw pointer to std::tm (const)
   * @return Pointer to std::tm or nullptr if not allocated
   */
  const std::tm* get() const { 
    return time_data_; 
  }
  
  /**
   * @brief Dereference operator (mutable) - allocates if needed
   * @return Reference to std::tm structure
   * @note This will allocate memory if not already done
   */
  std::tm& operator*() { 
    if (!time_data_) {
      time_data_ = allocate_tm();
    }
    return *time_data_; 
  }
  
  /**
   * @brief Dereference operator (const)
   * @return Reference to std::tm structure or static empty structure
   */
  const std::tm& operator*() const { 
    static std::tm empty_tm = {};
    return time_data_ ? *time_data_ : empty_tm;
  }
  
  /**
   * @brief Arrow operator (mutable) - allocates if needed
   * @return Pointer to std::tm structure
   * @note This will allocate memory if not already done
   */
  std::tm* operator->() { 
    if (!time_data_) {
      time_data_ = allocate_tm();
    }
    return time_data_; 
  }
  
  /**
   * @brief Arrow operator (const)
   * @return Pointer to std::tm structure or nullptr
   */
  const std::tm* operator->() const { 
    return time_data_; 
  }
  
  /**
   * @brief Check if time data is allocated and valid
   * @return true if time data is allocated
   */
  bool is_valid() const { 
    return time_data_ != nullptr; 
  }
  
  /**
   * @brief Convert to std::tm structure
   * @return Copy of std::tm or empty structure if not allocated
   */
  std::tm to_tm() const {
    return time_data_ ? *time_data_ : std::tm{};
  }
  
  /**
   * @brief Implicit conversion to std::tm
   * @return Copy of std::tm structure
   */
  operator std::tm() const {
    return to_tm();
  }
  
  /**
   * @brief Boolean conversion operator
   * @return true if time data is allocated and valid
   */
  operator bool() const {
    return is_valid();
  }
  
  /**
   * @brief Reset/clear the time data
   * This deallocates the memory and sets to uninitialized state
   */
  void reset() {
    deallocate_tm();
  }
  
  /**
   * @brief Check which memory type is being used
   * @return true if using PSRAM, false if using internal RAM
   */
  bool is_using_psram() const {
#ifdef ESP32
    if (!time_data_ || !psramFound()) return false;
    // Check if pointer is in PSRAM address range
    // ESP32-S3 PSRAM typically starts at 0x3C000000
    uintptr_t addr = reinterpret_cast<uintptr_t>(time_data_);
    return (addr >= 0x3C000000 && addr < 0x40000000);
#else
    return false;
#endif
  }
  
  /**
   * @brief Ensure time data is allocated
   * @return true if allocation successful or already allocated
   */
  bool ensure_allocated() {
    if (!time_data_) {
      time_data_ = allocate_tm();
    }
    return time_data_ != nullptr;
  }
};

}  // namespace cwa_town_forecast
}  // namespace esphome