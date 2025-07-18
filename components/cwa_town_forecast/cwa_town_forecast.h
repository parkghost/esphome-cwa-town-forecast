#pragma once

#include <cstdint>
#include <ctime>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "esphome.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/time.h"

namespace esphome {

// Add missing operator== for RAMAllocator to support std::vector move operations
template<class T> 
bool operator==(const RAMAllocator<T>& lhs, const RAMAllocator<T>& rhs) {
  return true;  // RAMAllocators are always equal for std::vector purposes
}

namespace cwa_town_forecast {

static const char *const TAG = "cwa_town_forecast";

static constexpr int UV_LOOKAHEAD_MINUTES = 90;

enum Mode {
  THREE_DAYS,
  SEVEN_DAYS,
};

// Memory management structures for transaction safety
struct MinimalCheckpoint {
  std::string locations_name;
  std::string location_name;
  double latitude;
  double longitude;
  Mode mode;
  size_t element_count;
  bool valid = false;
  
  MinimalCheckpoint() : latitude(NAN), longitude(NAN), mode(Mode::THREE_DAYS), element_count(0) {}
};

// Advanced Memory Management
enum class MemoryStrategy {
  PSRAM_DUAL_BUFFER,      // Use PSRAM for dual buffering
  INTERNAL_CHECKPOINT,    // Use internal RAM with minimal checkpoint
  ADAPTIVE_FRAGMENT,      // Adaptive strategy based on fragmentation
  STREAM_MINIMAL          // Ultra-minimal streaming approach
};

struct MemoryStats {
  size_t free_heap;
  size_t max_block;
  size_t total_heap;
  size_t psram_free;
  size_t psram_total;
  float fragmentation_ratio;
  bool has_psram;
  
  MemoryStats() : free_heap(0), max_block(0), total_heap(0), 
                  psram_free(0), psram_total(0), fragmentation_ratio(0.0f), has_psram(false) {}
};

class AdaptiveMemoryManager {
public:
  static constexpr size_t MIN_HEAP_FOR_TEMP_RECORD = 45000;  // Minimum heap for temp record strategy
  static constexpr size_t MIN_BLOCK_FOR_TEMP_RECORD = 20000; // Minimum block for temp record strategy
  static constexpr float MAX_FRAGMENTATION_RATIO = 0.7f;      // Maximum fragmentation before switching strategy
  
  static MemoryStats get_current_stats();
  static MemoryStrategy select_optimal_strategy(const MemoryStats& stats);
  static void log_memory_decision(MemoryStrategy strategy, const MemoryStats& stats);
  static bool should_use_psram_allocation(const MemoryStats& stats);
  static bool is_memory_sufficient_for_temp_record(const MemoryStats& stats);
};

static inline std::string mode_to_string(Mode mode) {
  switch (mode) {
    case Mode::THREE_DAYS:
      return "3-DAYS";
    case Mode::SEVEN_DAYS:
      return "7-DAYS";
    default:
      return "Unknown";
  }
}

enum EarlyDataClear {
  AUTO,
  ON,
  OFF,
};

static inline std::string early_data_clear_to_string(EarlyDataClear mode) {
  switch (mode) {
    case EarlyDataClear::AUTO:
      return "AUTO";
    case EarlyDataClear::ON:
      return "ON";
    case EarlyDataClear::OFF:
      return "OFF";
    default:
      return "Unknown";
  }
}

// Mapping of city names to their corresponding resource IDs
static const std::set<std::string> CITY_NAMES = {
    "宜蘭縣", "桃園市", "新竹縣", "苗栗縣", "彰化縣", "南投縣", "雲林縣", "嘉義縣", "屏東縣", "臺東縣", "花蓮縣",
    "澎湖縣", "基隆市", "新竹市", "嘉義市", "臺北市", "高雄市", "新北市", "臺中市", "臺南市", "連江縣", "金門縣",
};

static const char *const WEATHER_ELEMENT_NAME_TEMPERATURE = "溫度";
static const char *const WEATHER_ELEMENT_NAME_DEW_POINT = "露點溫度";
static const char *const WEATHER_ELEMENT_NAME_APPARENT_TEMPERATURE = "體感溫度";
static const char *const WEATHER_ELEMENT_NAME_COMFORT_INDEX = "舒適度指數";
static const char *const WEATHER_ELEMENT_NAME_WEATHER = "天氣現象";
static const char *const WEATHER_ELEMENT_NAME_WEATHER_DESCRIPTION = "天氣預報綜合描述";
static const char *const WEATHER_ELEMENT_NAME_3H_PROBABILITY_OF_PRECIPITATION = "3小時降雨機率";
static const char *const WEATHER_ELEMENT_NAME_RELATIVE_HUMIDITY = "相對濕度";
static const char *const WEATHER_ELEMENT_NAME_WIND_DIRECTION = "風向";
static const char *const WEATHER_ELEMENT_NAME_WIND_SPEED = "風速";
// New element names from 7 days forecast
static const char *const WEATHER_ELEMENT_NAME_AVG_TEMPERATURE = "平均溫度";
static const char *const WEATHER_ELEMENT_NAME_MAX_TEMPERATURE = "最高溫度";
static const char *const WEATHER_ELEMENT_NAME_MIN_TEMPERATURE = "最低溫度";
static const char *const WEATHER_ELEMENT_NAME_AVG_DEW_POINT = "平均露點溫度";
static const char *const WEATHER_ELEMENT_NAME_AVG_RELATIVE_HUMIDITY = "平均相對濕度";
static const char *const WEATHER_ELEMENT_NAME_MAX_APPARENT_TEMPERATURE = "最高體感溫度";
static const char *const WEATHER_ELEMENT_NAME_MIN_APPARENT_TEMPERATURE = "最低體感溫度";
static const char *const WEATHER_ELEMENT_NAME_MAX_COMFORT_INDEX = "最大舒適度指數";
static const char *const WEATHER_ELEMENT_NAME_MIN_COMFORT_INDEX = "最小舒適度指數";
static const char *const WEATHER_ELEMENT_NAME_12H_PROBABILITY_OF_PRECIPITATION = "12小時降雨機率";
static const char *const WEATHER_ELEMENT_NAME_UV_INDEX = "紫外線指數";

// Set of valid weather element names for 3 days forecasts
static const std::set<std::string> WEATHER_ELEMENT_NAMES_3DAYS = {
    WEATHER_ELEMENT_NAME_TEMPERATURE,
    WEATHER_ELEMENT_NAME_DEW_POINT,
    WEATHER_ELEMENT_NAME_APPARENT_TEMPERATURE,
    WEATHER_ELEMENT_NAME_COMFORT_INDEX,
    WEATHER_ELEMENT_NAME_WEATHER,
    WEATHER_ELEMENT_NAME_WEATHER_DESCRIPTION,
    WEATHER_ELEMENT_NAME_3H_PROBABILITY_OF_PRECIPITATION,
    WEATHER_ELEMENT_NAME_RELATIVE_HUMIDITY,
    WEATHER_ELEMENT_NAME_WIND_DIRECTION,
    WEATHER_ELEMENT_NAME_WIND_SPEED,
};

// Set of valid weather element names for 7 days forecast
static const std::set<std::string> WEATHER_ELEMENT_NAMES_7DAYS = {
    WEATHER_ELEMENT_NAME_AVG_TEMPERATURE,
    WEATHER_ELEMENT_NAME_MAX_TEMPERATURE,
    WEATHER_ELEMENT_NAME_MIN_TEMPERATURE,
    WEATHER_ELEMENT_NAME_AVG_DEW_POINT,
    WEATHER_ELEMENT_NAME_AVG_RELATIVE_HUMIDITY,
    WEATHER_ELEMENT_NAME_MAX_APPARENT_TEMPERATURE,
    WEATHER_ELEMENT_NAME_MIN_APPARENT_TEMPERATURE,
    WEATHER_ELEMENT_NAME_MAX_COMFORT_INDEX,
    WEATHER_ELEMENT_NAME_MIN_COMFORT_INDEX,
    WEATHER_ELEMENT_NAME_12H_PROBABILITY_OF_PRECIPITATION,
    WEATHER_ELEMENT_NAME_WEATHER,
    WEATHER_ELEMENT_NAME_WEATHER_DESCRIPTION,
    WEATHER_ELEMENT_NAME_WIND_DIRECTION,
    WEATHER_ELEMENT_NAME_WIND_SPEED,
    WEATHER_ELEMENT_NAME_UV_INDEX,
};

enum class ElementValueKey {
  TEMPERATURE,
  DEW_POINT,
  APPARENT_TEMPERATURE,
  COMFORT_INDEX,
  COMFORT_INDEX_DESCRIPTION,
  RELATIVE_HUMIDITY,
  WIND_DIRECTION,
  WIND_SPEED,
  BEAUFORT_SCALE,
  PROBABILITY_OF_PRECIPITATION,
  WEATHER,
  WEATHER_CODE,
  WEATHER_DESCRIPTION,
  WEATHER_ICON,
  // New keys from 7 days forecast
  MAX_TEMPERATURE,
  MIN_TEMPERATURE,
  MAX_APPARENT_TEMPERATURE,
  MIN_APPARENT_TEMPERATURE,
  MAX_COMFORT_INDEX,
  MAX_COMFORT_INDEX_DESCRIPTION,
  MIN_COMFORT_INDEX,
  MIN_COMFORT_INDEX_DESCRIPTION,
  UV_INDEX,
  UV_EXPOSURE_LEVEL
};

// Mapping of ElementValueKey enum to JSON field names
static const std::pair<ElementValueKey, const char *> ELEMENT_VALUE_KEY_NAMES[] = {
    {ElementValueKey::TEMPERATURE, "Temperature"},
    {ElementValueKey::DEW_POINT, "DewPoint"},
    {ElementValueKey::APPARENT_TEMPERATURE, "ApparentTemperature"},
    {ElementValueKey::COMFORT_INDEX, "ComfortIndex"},
    {ElementValueKey::COMFORT_INDEX_DESCRIPTION, "ComfortIndexDescription"},
    {ElementValueKey::RELATIVE_HUMIDITY, "RelativeHumidity"},
    {ElementValueKey::WIND_DIRECTION, "WindDirection"},
    {ElementValueKey::WIND_SPEED, "WindSpeed"},
    {ElementValueKey::BEAUFORT_SCALE, "BeaufortScale"},
    {ElementValueKey::PROBABILITY_OF_PRECIPITATION, "ProbabilityOfPrecipitation"},
    {ElementValueKey::WEATHER, "Weather"},
    {ElementValueKey::WEATHER_CODE, "WeatherCode"},
    {ElementValueKey::WEATHER_DESCRIPTION, "WeatherDescription"},
    {ElementValueKey::WEATHER_ICON, "WeatherIcon"},
    {ElementValueKey::MAX_TEMPERATURE, "MaxTemperature"},
    {ElementValueKey::MIN_TEMPERATURE, "MinTemperature"},
    {ElementValueKey::MAX_APPARENT_TEMPERATURE, "MaxApparentTemperature"},
    {ElementValueKey::MIN_APPARENT_TEMPERATURE, "MinApparentTemperature"},
    {ElementValueKey::MAX_COMFORT_INDEX, "MaxComfortIndex"},
    {ElementValueKey::MAX_COMFORT_INDEX_DESCRIPTION, "MaxComfortIndexDescription"},
    {ElementValueKey::MIN_COMFORT_INDEX, "MinComfortIndex"},
    {ElementValueKey::MIN_COMFORT_INDEX_DESCRIPTION, "MinComfortIndexDescription"},
    {ElementValueKey::UV_INDEX, "UVIndex"},
    {ElementValueKey::UV_EXPOSURE_LEVEL, "UVExposureLevel"}};

// Convert ElementValueKey enum to its corresponding string key
static inline std::string element_value_key_to_string(ElementValueKey key) {
  for (auto &p : ELEMENT_VALUE_KEY_NAMES) {
    if (p.first == key) return p.second;
  }
  return std::string();
}

// Map ElementValueKey to WeatherElementName based on Mode
static const std::unordered_map<Mode, std::unordered_map<ElementValueKey, std::string>> MODE_ELEMENT_NAME_MAP = {
    {Mode::THREE_DAYS,
     {
         {ElementValueKey::TEMPERATURE, WEATHER_ELEMENT_NAME_TEMPERATURE},
         {ElementValueKey::DEW_POINT, WEATHER_ELEMENT_NAME_DEW_POINT},
         {ElementValueKey::APPARENT_TEMPERATURE, WEATHER_ELEMENT_NAME_APPARENT_TEMPERATURE},
         {ElementValueKey::COMFORT_INDEX, WEATHER_ELEMENT_NAME_COMFORT_INDEX},
         {ElementValueKey::COMFORT_INDEX_DESCRIPTION, WEATHER_ELEMENT_NAME_COMFORT_INDEX},
         {ElementValueKey::WEATHER, WEATHER_ELEMENT_NAME_WEATHER},
         {ElementValueKey::WEATHER_CODE, WEATHER_ELEMENT_NAME_WEATHER},
         {ElementValueKey::WEATHER_ICON, WEATHER_ELEMENT_NAME_WEATHER},
         {ElementValueKey::WEATHER_DESCRIPTION, WEATHER_ELEMENT_NAME_WEATHER_DESCRIPTION},
         {ElementValueKey::PROBABILITY_OF_PRECIPITATION, WEATHER_ELEMENT_NAME_3H_PROBABILITY_OF_PRECIPITATION},
         {ElementValueKey::RELATIVE_HUMIDITY, WEATHER_ELEMENT_NAME_RELATIVE_HUMIDITY},
         {ElementValueKey::WIND_DIRECTION, WEATHER_ELEMENT_NAME_WIND_DIRECTION},
         {ElementValueKey::WIND_SPEED, WEATHER_ELEMENT_NAME_WIND_SPEED},
         {ElementValueKey::BEAUFORT_SCALE, WEATHER_ELEMENT_NAME_WIND_SPEED},
     }},
    {Mode::SEVEN_DAYS,
     {
         {ElementValueKey::TEMPERATURE, WEATHER_ELEMENT_NAME_AVG_TEMPERATURE},
         {ElementValueKey::DEW_POINT, WEATHER_ELEMENT_NAME_AVG_DEW_POINT},
         {ElementValueKey::RELATIVE_HUMIDITY, WEATHER_ELEMENT_NAME_AVG_RELATIVE_HUMIDITY},
         {ElementValueKey::MAX_TEMPERATURE, WEATHER_ELEMENT_NAME_MAX_TEMPERATURE},
         {ElementValueKey::MIN_TEMPERATURE, WEATHER_ELEMENT_NAME_MIN_TEMPERATURE},
         {ElementValueKey::MAX_APPARENT_TEMPERATURE, WEATHER_ELEMENT_NAME_MAX_APPARENT_TEMPERATURE},
         {ElementValueKey::MIN_APPARENT_TEMPERATURE, WEATHER_ELEMENT_NAME_MIN_APPARENT_TEMPERATURE},
         {ElementValueKey::MAX_COMFORT_INDEX, WEATHER_ELEMENT_NAME_MAX_COMFORT_INDEX},
         {ElementValueKey::MIN_COMFORT_INDEX, WEATHER_ELEMENT_NAME_MIN_COMFORT_INDEX},
         {ElementValueKey::MIN_COMFORT_INDEX_DESCRIPTION, WEATHER_ELEMENT_NAME_MIN_COMFORT_INDEX},
         {ElementValueKey::MAX_COMFORT_INDEX_DESCRIPTION, WEATHER_ELEMENT_NAME_MAX_COMFORT_INDEX},
         {ElementValueKey::PROBABILITY_OF_PRECIPITATION, WEATHER_ELEMENT_NAME_12H_PROBABILITY_OF_PRECIPITATION},
         {ElementValueKey::UV_INDEX, WEATHER_ELEMENT_NAME_UV_INDEX},
         {ElementValueKey::WEATHER, WEATHER_ELEMENT_NAME_WEATHER},
         {ElementValueKey::WEATHER_CODE, WEATHER_ELEMENT_NAME_WEATHER},
         {ElementValueKey::WEATHER_ICON, WEATHER_ELEMENT_NAME_WEATHER},
         {ElementValueKey::WEATHER_DESCRIPTION, WEATHER_ELEMENT_NAME_WEATHER_DESCRIPTION},
         {ElementValueKey::WIND_DIRECTION, WEATHER_ELEMENT_NAME_WIND_DIRECTION},
         {ElementValueKey::WIND_SPEED, WEATHER_ELEMENT_NAME_WIND_SPEED},
         {ElementValueKey::BEAUFORT_SCALE, WEATHER_ELEMENT_NAME_WIND_SPEED},
         {ElementValueKey::UV_EXPOSURE_LEVEL, WEATHER_ELEMENT_NAME_UV_INDEX},
     }},
};

struct WeatherCodeIcon {
  const char* code;
  const char* icon;
};

static constexpr WeatherCodeIcon WEATHER_CODE_TO_WEATHER_ICON_NAME_MAP[] = {
    {"01", "sunny"},             // mdi:weather-sunny
    {"02", "partly-cloudy"},     // mdi:weather-partly-cloudy
    {"03", "partly-cloudy"},     // mdi:weather-partly-cloudy
    {"04", "partly-cloudy"},     // mdi:weather-partly-cloudy
    {"05", "cloudy"},            // mdi:weather-cloudy
    {"06", "cloudy"},            // mdi:weather-cloudy
    {"07", "cloudy"},            // mdi:weather-cloudy
    {"08", "pouring"},           // mdi:weather-pouring
    {"09", "partly-rainy"},      // mdi:weather-partly-rainy
    {"10", "partly-rainy"},      // mdi:weather-partly-rainy
    {"11", "rainy"},             // mdi:weather-rainy
    {"12", "rainy"},             // mdi:weather-rainy
    {"13", "rainy"},             // mdi:weather-rainy
    {"14", "rainy"},             // mdi:weather-rainy
    {"15", "lightning-rainy"},   // mdi:weather-lightning-rainy
    {"16", "lightning-rainy"},   // mdi:weather-lightning-rainy
    {"17", "lightning-rainy"},   // mdi:weather-lightning-rainy
    {"18", "lightning-rainy"},   // mdi:weather-lightning-rainy
    {"19", "partly-rainy"},      // mdi:weather-partly-rainy
    {"20", "partly-rainy"},      // mdi:weather-partly-rainy
    {"21", "partly-lightning"},  // mdi:weather-partly-lightning
    {"22", "partly-lightning"},  // mdi:weather-partly-lightning
    {"23", "snowy-rainy"},       // mdi:weather-snowy-rainy
    {"24", "fog"},               // mdi:weather-fog
    {"25", "fog"},               // mdi:weather-fog
    {"26", "fog"},               // mdi:weather-fog
    {"27", "fog"},               // mdi:weather-fog
    {"28", "fog"},               // mdi:weather-fog
    {"29", "partly-rainy"},      // mdi:weather-partly-rainy
    {"30", "rainy"},             // mdi:weather-rainy
    {"31", "rainy"},             // mdi:weather-rainy
    {"32", "rainy"},             // mdi:weather-rainy
    {"33", "partly-lightning"},  // mdi:weather-partly-lightning
    {"34", "lightning-rainy"},   // mdi:weather-lightning-rainy
    {"35", "lightning-rainy"},   // mdi:weather-lightning-rainy
    {"36", "lightning-rainy"},   // mdi:weather-lightning-rainy
    {"37", "snowy-rainy"},       // mdi:weather-snowy-rainy
    {"38", "rainy"},             // mdi:weather-rainy
    {"39", "rainy"},             // mdi:weather-rainy
    {"41", "lightning-rainy"},   // mdi:weather-lightning-rainy
    {"42", "snowy"},             // mdi:weather-snowy
};

static constexpr size_t WEATHER_CODE_TO_WEATHER_ICON_NAME_MAP_SIZE = 
    sizeof(WEATHER_CODE_TO_WEATHER_ICON_NAME_MAP) / sizeof(WEATHER_CODE_TO_WEATHER_ICON_NAME_MAP[0]);

inline const char* find_weather_icon(const std::string& weather_code) {
  for (size_t i = 0; i < WEATHER_CODE_TO_WEATHER_ICON_NAME_MAP_SIZE; ++i) {
    if (weather_code == WEATHER_CODE_TO_WEATHER_ICON_NAME_MAP[i].code) {
      return WEATHER_CODE_TO_WEATHER_ICON_NAME_MAP[i].icon;
    }
  }
  return "";
}

struct CityResourcePair {
  const char* city;
  const char* resource_id;
};

static constexpr CityResourcePair CITY_NAME_TO_3D_RESOURCE_ID_MAP[] = {
    {"宜蘭縣", "F-D0047-001"}, {"桃園市", "F-D0047-005"}, {"新竹縣", "F-D0047-009"}, {"苗栗縣", "F-D0047-013"}, {"彰化縣", "F-D0047-017"},
    {"南投縣", "F-D0047-021"}, {"雲林縣", "F-D0047-025"}, {"嘉義縣", "F-D0047-029"}, {"屏東縣", "F-D0047-033"}, {"臺東縣", "F-D0047-037"},
    {"花蓮縣", "F-D0047-041"}, {"澎湖縣", "F-D0047-045"}, {"基隆市", "F-D0047-049"}, {"新竹市", "F-D0047-053"}, {"嘉義市", "F-D0047-057"},
    {"臺北市", "F-D0047-061"}, {"高雄市", "F-D0047-065"}, {"新北市", "F-D0047-069"}, {"臺中市", "F-D0047-073"}, {"臺南市", "F-D0047-077"},
    {"連江縣", "F-D0047-081"}, {"金門縣", "F-D0047-085"},
};

static constexpr size_t CITY_NAME_TO_3D_RESOURCE_ID_MAP_SIZE = 
    sizeof(CITY_NAME_TO_3D_RESOURCE_ID_MAP) / sizeof(CITY_NAME_TO_3D_RESOURCE_ID_MAP[0]);

static constexpr CityResourcePair CITY_NAME_TO_7D_RESOURCE_ID_MAP[] = {
    {"宜蘭縣", "F-D0047-003"}, {"桃園市", "F-D0047-007"}, {"新竹縣", "F-D0047-011"}, {"苗栗縣", "F-D0047-015"}, {"彰化縣", "F-D0047-019"},
    {"南投縣", "F-D0047-023"}, {"雲林縣", "F-D0047-027"}, {"嘉義縣", "F-D0047-031"}, {"屏東縣", "F-D0047-035"}, {"臺東縣", "F-D0047-039"},
    {"花蓮縣", "F-D0047-043"}, {"澎湖縣", "F-D0047-047"}, {"基隆市", "F-D0047-051"}, {"新竹市", "F-D0047-055"}, {"嘉義市", "F-D0047-059"},
    {"臺北市", "F-D0047-063"}, {"高雄市", "F-D0047-067"}, {"新北市", "F-D0047-071"}, {"臺中市", "F-D0047-075"}, {"臺南市", "F-D0047-079"},
    {"連江縣", "F-D0047-083"}, {"金門縣", "F-D0047-087"},
};

static constexpr size_t CITY_NAME_TO_7D_RESOURCE_ID_MAP_SIZE = 
    sizeof(CITY_NAME_TO_7D_RESOURCE_ID_MAP) / sizeof(CITY_NAME_TO_7D_RESOURCE_ID_MAP[0]);

inline const char* find_city_resource_id(const std::string& city_name, bool is_7_days) {
  if (is_7_days) {
    for (size_t i = 0; i < CITY_NAME_TO_7D_RESOURCE_ID_MAP_SIZE; ++i) {
      if (city_name == CITY_NAME_TO_7D_RESOURCE_ID_MAP[i].city) {
        return CITY_NAME_TO_7D_RESOURCE_ID_MAP[i].resource_id;
      }
    }
  } else {
    for (size_t i = 0; i < CITY_NAME_TO_3D_RESOURCE_ID_MAP_SIZE; ++i) {
      if (city_name == CITY_NAME_TO_3D_RESOURCE_ID_MAP[i].city) {
        return CITY_NAME_TO_3D_RESOURCE_ID_MAP[i].resource_id;
      }
    }
  }
  return "";
}

static inline std::tm mktm(int year, int month, int day, int hour, int minute, int second) {
  std::tm tm{};
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = second;
  return tm;
}

// Parse a string key to ElementValueKey enum
static inline bool parse_element_value_key(const std::string &key, ElementValueKey &out) {
  for (auto &p : ELEMENT_VALUE_KEY_NAMES) {
    if (key == p.second) {
      out = p.first;
      return true;
    }
  }
  return false;
}

// Convert a std::tm struct to ESPTime
static inline ESPTime tm_to_esptime(const std::tm tm) {
  std::tm t = tm;
  std::time_t epoch = std::mktime(&t);
  return ESPTime::from_c_tm(&t, epoch);
}

struct Time {
  std::unique_ptr<std::tm> data_time;
  std::unique_ptr<std::tm> start_time;
  std::unique_ptr<std::tm> end_time;
  std::vector<std::pair<ElementValueKey, std::string>, RAMAllocator<std::pair<ElementValueKey, std::string>>> element_values;

  Time() : element_values(RAMAllocator<std::pair<ElementValueKey, std::string>>(RAMAllocator<std::pair<ElementValueKey, std::string>>::NONE)) {}
  Time(const Time &o) {
    if (o.data_time) data_time.reset(new std::tm(*o.data_time));
    if (o.start_time) start_time.reset(new std::tm(*o.start_time));
    if (o.end_time) end_time.reset(new std::tm(*o.end_time));
    for (const auto &p : o.element_values) element_values.push_back(p);
  }
  Time &operator=(const Time &o) {
    if (this == &o) return *this;
    if (o.data_time)
      data_time.reset(new std::tm(*o.data_time));
    else
      data_time.reset();
    if (o.start_time)
      start_time.reset(new std::tm(*o.start_time));
    else
      start_time.reset();
    if (o.end_time)
      end_time.reset(new std::tm(*o.end_time));
    else
      end_time.reset();
    element_values.clear();
    for (const auto &p : o.element_values) element_values.push_back(p);
    return *this;
  }
  Time(Time &&) = default;
  Time &operator=(Time &&) = default;

  std::string find_element_value(ElementValueKey key) const {
    for (const auto &p : this->element_values) {
      if (p.first == key) return p.second;
    }
    return std::string();
  }

  std::tm to_tm() const {
    if (this->data_time)
      return *this->data_time;
    else if (this->start_time)
      return *this->start_time;
    else
      return std::tm{};
  }

  ESPTime to_esptime() const { return tm_to_esptime(this->to_tm()); }
};

// Utility function to get min and max value for a given ElementValueKey in a vector of Time
template <typename Alloc>
inline std::pair<double, double> get_min_max_element_value(const std::vector<Time, Alloc> &times, ElementValueKey key) {
  using std::begin;
  using std::end;
  double min_val = 0.0, max_val = 0.0;
  bool found = false;
  for (const auto &t : times) {
    std::string val = t.find_element_value(key);
    if (!val.empty()) {
      char *endptr = nullptr;
      double num = std::strtod(val.c_str(), &endptr);
      if (endptr == val.c_str() || *endptr != '\0') {
        continue;  // skip if not a valid number
      }
      if (!found) {
        min_val = max_val = num;
        found = true;
      } else {
        if (num < min_val) min_val = num;
        if (num > max_val) max_val = num;
      }
    }
  }
  return {min_val, max_val};
}

struct WeatherElement {
  std::string element_name;
  std::vector<Time, RAMAllocator<Time>> times;
  WeatherElement() : times(RAMAllocator<Time>(RAMAllocator<Time>::NONE)) {}

  std::vector<Time, RAMAllocator<Time>> filter_times(const std::tm &start, const std::tm &end) const {
    std::vector<Time, RAMAllocator<Time>> result(RAMAllocator<Time>(RAMAllocator<Time>::NONE));
    std::tm start_copy = start, end_copy = end;
    std::time_t start_epoch = std::mktime(&start_copy);
    std::time_t end_epoch = std::mktime(&end_copy);
    for (const auto &t : this->times) {
      if (t.data_time) {
        std::tm dt = *t.data_time;
        std::time_t dt_epoch = std::mktime(&dt);
        if (dt_epoch >= start_epoch && dt_epoch < end_epoch) {
          result.push_back(t);
        }
      } else if (t.start_time && t.end_time) {
        std::tm st = *t.start_time;
        std::tm en = *t.end_time;
        std::time_t st_epoch = std::mktime(&st);
        std::time_t en_epoch = std::mktime(&en);
        // check if interval overlaps [start, end)
        if (en_epoch > start_epoch && st_epoch < end_epoch) {
          result.push_back(t);
        }
      }
    }
    return result;
  }

  Time *find_closest_time(const std::tm &target) const {
    if (this->times.empty()) return nullptr;

    size_t closest_idx = 0;
    std::tm tgt = target;
    std::time_t tgt_epoch = std::mktime(&tgt);
    std::time_t min_diff = std::numeric_limits<std::time_t>::max();
    for (size_t i = 0; i < this->times.size(); ++i) {
      std::tm t_tm = this->times[i].to_tm();
      std::time_t t_epoch = std::mktime(&t_tm);
      std::time_t diff = (t_epoch > tgt_epoch) ? t_epoch - tgt_epoch : tgt_epoch - t_epoch;
      if (diff < min_diff) {
        min_diff = diff;
        closest_idx = i;
      }
    }
    return const_cast<Time *>(&this->times[closest_idx]);
  }

  Time *match_time(const std::tm &target, ElementValueKey key, bool fallback_to_first_element) const {
    if (this->times.empty()) return nullptr;

    Time *best = nullptr;
    std::tm tgt = target;
    std::time_t tgt_epoch = std::mktime(&tgt);
    for (const auto &t : this->times) {
      if (t.data_time) {
        std::tm dt = *t.data_time;
        std::time_t dt_epoch = std::mktime(&dt);
        if (dt_epoch <= tgt_epoch) {
          best = const_cast<Time *>(&t);
        }
      } else if (t.start_time && t.end_time) {
        std::tm st = *t.start_time;
        std::tm en = *t.end_time;
        std::time_t st_epoch = std::mktime(&st);
        std::time_t en_epoch = std::mktime(&en);
        if (tgt_epoch >= st_epoch && tgt_epoch < en_epoch) {
          return const_cast<Time *>(&t);
        }
      }
    }
    if (best == nullptr && fallback_to_first_element) {
      best = const_cast<Time *>(&this->times[0]);
      if (key == ElementValueKey::UV_EXPOSURE_LEVEL || key == ElementValueKey::UV_INDEX) {
        if (best->start_time) {
          std::tm st = *best->start_time;
          std::tm tgt = target;
          std::time_t st_epoch = std::mktime(&st);
          std::time_t tm_epoch = std::mktime(&tgt);

          if (st_epoch > tm_epoch && (st_epoch - tm_epoch) > UV_LOOKAHEAD_MINUTES * 60) {
            ESP_LOGD(TAG, "UV data fallback outside of the forecast window");
            return nullptr;
          }
        }
      }
      ESP_LOGW(TAG, "No matching time found for %s, using first element :%s", this->element_name.c_str(),
               best->to_esptime().strftime("%Y-%m-%d %H:%M").c_str());
    }
    return best;
  }
};

struct Record {
  Mode mode;
  std::string locations_name;
  std::string location_name;
  std::tm start_time;
  std::tm end_time;
  std::tm updated_time;
  double latitude;
  double longitude;
  std::vector<WeatherElement, RAMAllocator<WeatherElement>> weather_elements;

  Record() : weather_elements(RAMAllocator<WeatherElement>(RAMAllocator<WeatherElement>::NONE)) {}

  const WeatherElement *find_weather_element(const std::string &name) const {
    for (const auto &we : weather_elements) {
      if (we.element_name == name) return &we;
    }
    return nullptr;
  }

  const WeatherElement *get_weather_element_for_key(ElementValueKey key) const {
    auto mode_it = MODE_ELEMENT_NAME_MAP.find(this->mode);
    if (mode_it == MODE_ELEMENT_NAME_MAP.end()) return nullptr;
    const auto &elem_map = mode_it->second;
    auto name_it = elem_map.find(key);
    if (name_it == elem_map.end()) return nullptr;
    return this->find_weather_element(name_it->second);
  }

  const std::string find_value(ElementValueKey key, bool fallback_to_first_element, std::tm tm) const {
    return find_value(key, fallback_to_first_element, tm, std::string("-"));
  }

  const std::string find_value(ElementValueKey key, bool fallback_to_first_element, std::tm tm, std::string default_value) const {
    const WeatherElement *we = this->get_weather_element_for_key(key);
    if (!we) return default_value;
    if (Time *ts = we->match_time(tm, key, fallback_to_first_element)) {
      auto val = ts->find_element_value(key);
      return val.empty() ? default_value : val;
    }
    return default_value;
  }

  const std::pair<double, double> find_min_max_values(ElementValueKey key, std::tm &start, std::tm &end) const {
    const WeatherElement *we = this->get_weather_element_for_key(key);
    if (!we) return std::make_pair(0.0, 0.0);
    return get_min_max_element_value(we->filter_times(start, end), key);
  }

  void dump() const {
    ESP_LOGI(TAG, "Record Dump:");
    ESP_LOGI(TAG, "  Mode: %s", mode_to_string(this->mode).c_str());
    ESP_LOGI(TAG, "  Locations Name: %s", this->locations_name.c_str());
    ESP_LOGI(TAG, "  Location Name: %s", this->location_name.c_str());
    ESP_LOGI(TAG, "  Latitude: %.6f", this->latitude);
    ESP_LOGI(TAG, "  Longitude: %.6f", this->longitude);
    ESP_LOGI(TAG, "  Start Time: %s", tm_to_esptime(this->start_time).strftime("%Y-%m-%dT%H:%M:%S").c_str());
    ESP_LOGI(TAG, "  End Time: %s", tm_to_esptime(this->end_time).strftime("%Y-%m-%dT%H:%M:%S").c_str());
    ESP_LOGI(TAG, "  Updated Time: %s", tm_to_esptime(this->updated_time).strftime("%Y-%m-%dT%H:%M:%S").c_str());
    for (const auto &we : this->weather_elements) {
      ESP_LOGI(TAG, "  Weather Element: %s", we.element_name.c_str());
      for (const auto &t : we.times) {
        std::string datetime_str = "";
        if (t.data_time) {
          datetime_str = tm_to_esptime(*t.data_time).strftime("%Y-%m-%dT%H:%M:%S");
        } else if (t.start_time && t.end_time) {
          datetime_str = tm_to_esptime(*t.start_time).strftime("%Y-%m-%dT%H:%M:%S") + " - " + tm_to_esptime(*t.end_time).strftime("%Y-%m-%dT%H:%M:%S");
        }

        std::string joined_values;
        for (const auto &kv : t.element_values) {
          if (!joined_values.empty()) {
            joined_values += ", ";
          }
          joined_values += element_value_key_to_string(kv.first) + "=" + kv.second;
        }

        ESP_LOGI(TAG, "    Time: %s    %s", datetime_str.c_str(), joined_values.c_str());
        delay(2);
      }
    }
  }
};

// Buffered stream class that pre-reads data into internal buffer
class BufferedStream : public Stream {
 public:
  static constexpr size_t MIN_BUFFER_SIZE = 64;
  static constexpr size_t MAX_BUFFER_SIZE = 4096;
  static constexpr size_t DEFAULT_BUFFER_SIZE = 512;
  
  BufferedStream(Stream& stream, size_t bufferSize = DEFAULT_BUFFER_SIZE) 
    : stream_(stream), bytes_read_(0), buffer_pos_(0), buffer_len_(0) {
    
    // Validate and clamp buffer size
    if (bufferSize < MIN_BUFFER_SIZE) {
      ESP_LOGW(TAG, "Buffer size %zu too small, using minimum %zu", bufferSize, MIN_BUFFER_SIZE);
      bufferSize = MIN_BUFFER_SIZE;
    } else if (bufferSize > MAX_BUFFER_SIZE) {
      ESP_LOGW(TAG, "Buffer size %zu too large, using maximum %zu", bufferSize, MAX_BUFFER_SIZE);
      bufferSize = MAX_BUFFER_SIZE;
    }
    
    buffer_.reserve(bufferSize);
    buffer_.resize(bufferSize);
    if (buffer_.size() != bufferSize) {
      ESP_LOGE(TAG, "Failed to allocate buffer for BufferedStream (requested: %zu, got: %zu)", 
               bufferSize, buffer_.size());
      buffer_.clear();
    } else {
      ESP_LOGD(TAG, "BufferedStream created with buffer size: %zu", buffer_.size());
    }
  }
  
  // Explicitly delete copy operations to prevent issues
  BufferedStream(const BufferedStream&) = delete;
  BufferedStream& operator=(const BufferedStream&) = delete;
  
  // Allow move operations
  BufferedStream(BufferedStream&&) = default;
  BufferedStream& operator=(BufferedStream&&) = default;
  
  ~BufferedStream() = default;
  
  // Simple health check based on buffer availability
  bool isHealthy() const { return !buffer_.empty(); }
  
  int available() override {
    // Return buffered data + underlying stream data
    return (buffer_len_ - buffer_pos_) + stream_.available();
  }
  
  int read() override {
    // If no buffer allocated, delegate to underlying stream
    if (buffer_.empty()) {
      int byte = stream_.read();
      if (byte != -1) {
        bytes_read_++;
      }
      return byte;
    }
    
    // If buffer is empty, try to fill it
    if (buffer_pos_ >= buffer_len_) {
      if (!fillBuffer()) {
        // Fall back to direct stream reading
        int byte = stream_.read();
        if (byte != -1) {
          bytes_read_++;
        }
        return byte;
      }
    }
    
    // Return from buffer if available
    if (buffer_pos_ < buffer_len_) {
      int byte = buffer_[buffer_pos_++];
      bytes_read_++;
      
      // Only log on significant milestones or special debug mode
      #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERY_VERBOSE
      if (bytes_read_ % 500 == 0 || (byte == '{' && bytes_read_ < 50) || 
          (byte == '}' && bytes_read_ > 100)) {
        ESP_LOGVV(TAG, "Read byte %d: 0x%02X ('%c') at position %zu", 
                  byte, byte, (byte >= 32 && byte <= 126) ? (char)byte : '.', bytes_read_);
      }
      #endif
      
      return byte;
    }
    
    return -1; // No data available
  }
  
  int peek() override {
    // If buffer is empty, try to fill it
    if (buffer_pos_ >= buffer_len_) {
      fillBuffer();
    }
    
    // Return from buffer if available
    if (buffer_pos_ < buffer_len_) {
      return buffer_[buffer_pos_];
    }
    
    return -1; // No data available
  }
  
  void flush() override { stream_.flush(); }
  size_t write(uint8_t byte) override { return stream_.write(byte); }
  
  // Delegate other important methods (bypass buffer for these operations)
  void setTimeout(unsigned long timeout) { stream_.setTimeout(timeout); }
  bool find(char* target) { 
    ESP_LOGV(TAG, "find() bypassing buffer, delegating to underlying stream");
    return stream_.find(target); 
  }
  bool findUntil(char* target, char* terminator) { 
    ESP_LOGV(TAG, "findUntil() bypassing buffer, delegating to underlying stream");
    return stream_.findUntil(target, terminator); 
  }
  
  String readStringUntil(char terminator) {
    String result;
    result.reserve(128); // Pre-allocate reasonable capacity
    
    int c;
    while ((c = read()) != -1) {
      if (c == terminator) break;
      result += (char)c;
      
      // Prevent runaway strings
      if (result.length() > 1024) {
        ESP_LOGW(TAG, "readStringUntil('%c') exceeded 1024 chars, truncating", terminator);
        break;
      }
    }
    
    ESP_LOGV(TAG, "readStringUntil('%c') returned: %s (length: %d)", 
             terminator, result.c_str(), result.length());
    return result;
  }
  
  size_t getBytesRead() const { return bytes_read_; }
  size_t getBufferSize() const { return buffer_.size(); }
  size_t getBufferedBytes() const { return buffer_len_ - buffer_pos_; }
  
  // Additional monitoring methods
  float getBufferUtilization() const { 
    return buffer_.empty() ? 0.0f : (float)buffer_len_ / (float)buffer_.size(); 
  }
  
  bool hasBufferedData() const { return buffer_pos_ < buffer_len_; }
  
  // Debug information
  void logBufferStats() const {
    ESP_LOGD(TAG, "Buffer stats: size=%zu, pos=%zu, len=%zu, utilization=%.1f%%, healthy=%s", 
             buffer_.size(), buffer_pos_, buffer_len_, 
             getBufferUtilization() * 100.0f, isHealthy() ? "true" : "false");
  }
  
  // Drain remaining buffered data to avoid SSL connection state issues
  void drainBuffer() {
    size_t remaining = buffer_len_ - buffer_pos_;
    if (remaining > 0) {
      ESP_LOGD(TAG, "Draining %d remaining bytes from buffer", remaining);
      buffer_pos_ = buffer_len_; // Mark buffer as empty
    }
    
    // Also drain any remaining data from underlying stream
    static constexpr int DRAIN_LIMIT = 200;
    int drained = 0;
    while (stream_.available() && drained < DRAIN_LIMIT) {
      int byte = stream_.read();
      if (byte == -1) break;
      drained++;
    }
    if (drained > 0) {
      ESP_LOGD(TAG, "Drained %d bytes from underlying stream", drained);
    }
  }
  
 private:
  bool fillBuffer() {
    if (buffer_.empty()) {
      // No buffer available, can't fill
      return false;
    }
    
    buffer_pos_ = 0;
    buffer_len_ = 0;
    
    // Read data into buffer, but don't over-read to avoid SSL state issues
    size_t available = stream_.available();
    if (available == 0) {
      return false; // No data to read
    }
    
    // More efficient reading - read in chunks when possible
    size_t read_limit = std::min(buffer_.size(), available);
    
    // Try to read data efficiently
    size_t bytes_to_read = std::min(read_limit, buffer_.size());
    
    for (size_t i = 0; i < bytes_to_read; i++) {
      int byte = stream_.read();
      if (byte == -1) break;
      buffer_[buffer_len_++] = static_cast<uint8_t>(byte);
    }
    
    if (buffer_len_ > 0) {
      #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERY_VERBOSE
      ESP_LOGVV(TAG, "Filled buffer with %zu bytes (available: %zu)", buffer_len_, available);
      #endif
      return true;
    }
    
    return false;
  }
  
  Stream& stream_;
  size_t bytes_read_;
  std::vector<uint8_t> buffer_;
  size_t buffer_pos_;  // Current position in buffer
  size_t buffer_len_;  // Amount of data in buffer
};

class CWATownForecast : public PollingComponent {
 public:
  float get_setup_priority() const override;

  void setup() override;

  void update() override;

  void dump_config() override;

  void set_time(time::RealTimeClock *rtc) { rtc_ = rtc; }

  void set_mode(Mode mode) { mode_ = mode; }

  template <typename V>
  void set_api_key(V key) {
    api_key_ = key;
  }

  template <typename V>
  void set_city_name(V city_name) {
    city_name_ = city_name;
  }

  template <typename V>
  void set_town_name(V town_name) {
    town_name_ = town_name;
  }

  void add_weather_element(const std::string &weather_element) { this->weather_elements_.insert(weather_element); }

  void set_weather_elements(const std::set<std::string> &weather_elements) { this->weather_elements_ = weather_elements; }

  template <typename V>
  void set_time_to(V time_to) {
    time_to_ = time_to;
  }

  template <typename V>
  void set_sensor_expiry(V expiry) {
    sensor_expiry_ = expiry;
  }

  template <typename V>
  void set_fallback_to_first_element(V fallback) {
    fallback_to_first_element_ = fallback;
  }

  template <typename V>
  void set_retain_fetched_data(V retain) {
    retain_fetched_data_ = retain;
  }

  template <typename V>
  void set_early_data_clear(V early_data_clear) {
    early_data_clear_ = early_data_clear;
  }

  template <typename V>
  void set_watchdog_timeout(V watchdog_timeout) {
    watchdog_timeout_ = watchdog_timeout;
  }

  template <typename V>
  void set_http_connect_timeout(V http_connect_timeout) {
    http_connect_timeout_ = http_connect_timeout;
  }

  template <typename V>
  void set_http_timeout(V http_timeout) {
    http_timeout_ = http_timeout;
  }

  template <typename V>
  void set_retry_count(V retry_count) {
    retry_count_ = retry_count;
  }

  template <typename V>
  void set_retry_delay(V retry_delay) {
    retry_delay_ = retry_delay;
  }

  void clear_data() { record_.weather_elements.clear(); }

  Record &get_data();

  void set_city_text_sensor(text_sensor::TextSensor *city) { city_sensor_ = city; }
  void set_town_text_sensor(text_sensor::TextSensor *town) { town_sensor_ = town; }
  void set_last_updated_text_sensor(text_sensor::TextSensor *sensor) { last_updated_ = sensor; }
  void set_last_success_text_sensor(text_sensor::TextSensor *sensor) { last_success_ = sensor; }
  void set_last_error_text_sensor(text_sensor::TextSensor *sensor) { last_error_ = sensor; }
  void set_temperature_sensor(sensor::Sensor *sensor) { temperature_ = sensor; }
  void set_dew_point_sensor(sensor::Sensor *sensor) { dew_point_ = sensor; }
  void set_apparent_temperature_sensor(sensor::Sensor *sensor) { apparent_temperature_ = sensor; }
  void set_comfort_index_text_sensor(text_sensor::TextSensor *sensor) { comfort_index_ = sensor; }
  void set_comfort_index_description_text_sensor(text_sensor::TextSensor *sensor) { comfort_index_description_ = sensor; }
  void set_relative_humidity_sensor(sensor::Sensor *sensor) { relative_humidity_ = sensor; }
  void set_wind_speed_sensor(sensor::Sensor *sensor) { wind_speed_ = sensor; }
  void set_probability_of_precipitation_sensor(sensor::Sensor *sensor) { probability_of_precipitation_ = sensor; }
  void set_weather_text_sensor(text_sensor::TextSensor *sensor) { weather_ = sensor; }
  void set_weather_code_text_sensor(text_sensor::TextSensor *sensor) { weather_code_ = sensor; }
  void set_weather_description_text_sensor(text_sensor::TextSensor *sensor) { weather_description_ = sensor; }
  void set_weather_icon_text_sensor(text_sensor::TextSensor *sensor) { weather_icon_ = sensor; }
  void set_wind_direction_text_sensor(text_sensor::TextSensor *sensor) { wind_direction_ = sensor; }
  void set_beaufort_scale_text_sensor(text_sensor::TextSensor *sensor) { beaufort_scale_ = sensor; }
  void set_max_temperature_sensor(sensor::Sensor *sensor) { max_temperature_ = sensor; }
  void set_min_temperature_sensor(sensor::Sensor *sensor) { min_temperature_ = sensor; }
  void set_max_apparent_temperature_sensor(sensor::Sensor *sensor) { max_apparent_temperature_ = sensor; }
  void set_min_apparent_temperature_sensor(sensor::Sensor *sensor) { min_apparent_temperature_ = sensor; }
  void set_max_comfort_index_text_sensor(text_sensor::TextSensor *sensor) { max_comfort_index_ = sensor; }
  void set_min_comfort_index_text_sensor(text_sensor::TextSensor *sensor) { min_comfort_index_ = sensor; }
  void set_max_comfort_index_description_text_sensor(text_sensor::TextSensor *sensor) { max_comfort_index_description_ = sensor; }
  void set_min_comfort_index_description_text_sensor(text_sensor::TextSensor *sensor) { min_comfort_index_description_ = sensor; }
  void set_uv_index_sensor(sensor::Sensor *sensor) { uv_index_ = sensor; }
  void set_uv_exposure_level_text_sensor(text_sensor::TextSensor *sensor) { uv_exposure_level_ = sensor; }
  Trigger<Record &> *get_on_data_change_trigger() { return &this->on_data_change_trigger_; }
  Trigger<> *get_on_error_trigger() { return &this->on_error_trigger_; }

 protected:
  // Memory management helper methods
  MinimalCheckpoint create_minimal_checkpoint(const Record& record);
  void restore_from_checkpoint(Record& record, const MinimalCheckpoint& checkpoint);
  bool parse_to_record(Stream &stream, Record& record, uint64_t &hash_code);
  
  // Advanced memory management methods
  bool process_response_with_adaptive_strategy(Stream &stream, uint64_t &hash_code);
  bool process_with_psram_dual_buffer(Stream &stream, Record& record, uint64_t &hash_code);
  bool process_with_internal_checkpoint(Stream &stream, Record& record, uint64_t &hash_code);
  bool process_with_adaptive_fragment(Stream &stream, Record& record, uint64_t &hash_code);
  bool process_with_stream_minimal(Stream &stream, Record& record, uint64_t &hash_code);
  
  TemplatableValue<std::string> api_key_;
  TemplatableValue<std::string> city_name_;
  TemplatableValue<std::string> town_name_;
  Mode mode_;
  std::set<std::string> weather_elements_;
  TemplatableValue<uint32_t> time_to_;
  TemplatableValue<EarlyDataClear> early_data_clear_;
  TemplatableValue<bool> fallback_to_first_element_;
  TemplatableValue<bool> retain_fetched_data_;
  TemplatableValue<uint32_t> sensor_expiry_;
  TemplatableValue<uint32_t> watchdog_timeout_;
  TemplatableValue<uint32_t> http_connect_timeout_;
  TemplatableValue<uint32_t> http_timeout_;
  TemplatableValue<uint32_t> retry_count_;
  TemplatableValue<uint32_t> retry_delay_;
  time::RealTimeClock *rtc_{nullptr};

  text_sensor::TextSensor *city_sensor_{nullptr};
  text_sensor::TextSensor *town_sensor_{nullptr};
  text_sensor::TextSensor *last_updated_{nullptr};
  text_sensor::TextSensor *last_success_{nullptr};
  text_sensor::TextSensor *last_error_{nullptr};
  sensor::Sensor *temperature_{nullptr};
  sensor::Sensor *dew_point_{nullptr};
  sensor::Sensor *apparent_temperature_{nullptr};
  text_sensor::TextSensor *comfort_index_description_{nullptr};
  sensor::Sensor *relative_humidity_{nullptr};
  sensor::Sensor *wind_speed_{nullptr};
  sensor::Sensor *probability_of_precipitation_{nullptr};
  text_sensor::TextSensor *weather_{nullptr};
  text_sensor::TextSensor *weather_code_{nullptr};
  text_sensor::TextSensor *weather_description_{nullptr};
  text_sensor::TextSensor *weather_icon_{nullptr};
  text_sensor::TextSensor *comfort_index_{nullptr};
  text_sensor::TextSensor *wind_direction_{nullptr};
  text_sensor::TextSensor *beaufort_scale_{nullptr};

  // New Sensor from 7 days forecast
  sensor::Sensor *max_temperature_{nullptr};
  sensor::Sensor *min_temperature_{nullptr};
  sensor::Sensor *max_apparent_temperature_{nullptr};
  sensor::Sensor *min_apparent_temperature_{nullptr};
  text_sensor::TextSensor *max_comfort_index_{nullptr};
  text_sensor::TextSensor *min_comfort_index_{nullptr};
  text_sensor::TextSensor *max_comfort_index_description_{nullptr};
  text_sensor::TextSensor *min_comfort_index_description_{nullptr};
  sensor::Sensor *uv_index_{nullptr};
  text_sensor::TextSensor *uv_exposure_level_{nullptr};

  Trigger<Record &> on_data_change_trigger_{};
  Trigger<> on_error_trigger_{};

  uint64_t last_hash_code_{0};
  Record record_;
  time_t sensor_expiration_time_{};

  bool send_request_();
  bool send_request_with_retry_();
  bool validate_config_();
  bool process_response_(Stream &stream, uint64_t &hash_code);
  bool check_changes(uint64_t new_hash_code);
  void publish_states_();
  void publish_sensor_state_(sensor::Sensor *sensor, ElementValueKey key, std::tm &target_tm, bool fallback_to_first);
  void publish_text_sensor_state_(text_sensor::TextSensor *sensor, ElementValueKey key, std::tm &target_tm, bool fallback_to_first);
  template <typename SensorT, typename PublishValFunc, typename PublishNoMatchFunc>
  void publish_state_common_(SensorT *sensor, ElementValueKey key, std::tm &target_tm, bool fallback_to_first, PublishValFunc publish_val,
                             PublishNoMatchFunc publish_no_match);
};

}  // namespace cwa_town_forecast
}  // namespace esphome