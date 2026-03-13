#pragma once

#include <cstdint>
#include <ctime>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "esphome.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/time.h"

#ifdef USE_ESP32
#include "esp_heap_caps.h"
#ifdef USE_PSRAM
#include "esp_psram.h"
#endif
#endif

#include "esphome/components/http_request/http_request.h"
#include "psram_string.h"
#include "psram_time.h"
#include "http_stream_adapter.h"
#include "url_encode.h"

#ifdef USE_PSRAM
#define CWA_PSRAM_AVAILABLE() esp_psram_is_initialized()
#else
#define CWA_PSRAM_AVAILABLE() false
#endif

namespace esphome {

// Add missing operator== for RAMAllocator to support std::vector move operations
template<class T>
bool operator==(const RAMAllocator<T>& lhs, const RAMAllocator<T>& rhs) {
  return true;  // RAMAllocators are always equal for std::vector purposes
}

namespace cwa_town_forecast {

static constexpr const char *const TAG = "cwa_town_forecast";

static constexpr int UV_LOOKAHEAD_MINUTES = 90;

enum Mode {
  THREE_DAYS,
  SEVEN_DAYS,
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

static constexpr const char *const WEATHER_ELEMENT_NAME_TEMPERATURE = "溫度";
static constexpr const char *const WEATHER_ELEMENT_NAME_DEW_POINT = "露點溫度";
static constexpr const char *const WEATHER_ELEMENT_NAME_APPARENT_TEMPERATURE = "體感溫度";
static constexpr const char *const WEATHER_ELEMENT_NAME_COMFORT_INDEX = "舒適度指數";
static constexpr const char *const WEATHER_ELEMENT_NAME_WEATHER = "天氣現象";
static constexpr const char *const WEATHER_ELEMENT_NAME_WEATHER_DESCRIPTION = "天氣預報綜合描述";
static constexpr const char *const WEATHER_ELEMENT_NAME_3H_PROBABILITY_OF_PRECIPITATION = "3小時降雨機率";
static constexpr const char *const WEATHER_ELEMENT_NAME_RELATIVE_HUMIDITY = "相對濕度";
static constexpr const char *const WEATHER_ELEMENT_NAME_WIND_DIRECTION = "風向";
static constexpr const char *const WEATHER_ELEMENT_NAME_WIND_SPEED = "風速";
// New element names from 7 days forecast
static constexpr const char *const WEATHER_ELEMENT_NAME_AVG_TEMPERATURE = "平均溫度";
static constexpr const char *const WEATHER_ELEMENT_NAME_MAX_TEMPERATURE = "最高溫度";
static constexpr const char *const WEATHER_ELEMENT_NAME_MIN_TEMPERATURE = "最低溫度";
static constexpr const char *const WEATHER_ELEMENT_NAME_AVG_DEW_POINT = "平均露點溫度";
static constexpr const char *const WEATHER_ELEMENT_NAME_AVG_RELATIVE_HUMIDITY = "平均相對濕度";
static constexpr const char *const WEATHER_ELEMENT_NAME_MAX_APPARENT_TEMPERATURE = "最高體感溫度";
static constexpr const char *const WEATHER_ELEMENT_NAME_MIN_APPARENT_TEMPERATURE = "最低體感溫度";
static constexpr const char *const WEATHER_ELEMENT_NAME_MAX_COMFORT_INDEX = "最大舒適度指數";
static constexpr const char *const WEATHER_ELEMENT_NAME_MIN_COMFORT_INDEX = "最小舒適度指數";
static constexpr const char *const WEATHER_ELEMENT_NAME_12H_PROBABILITY_OF_PRECIPITATION = "12小時降雨機率";
static constexpr const char *const WEATHER_ELEMENT_NAME_UV_INDEX = "紫外線指數";

// Valid weather element names for 3 days forecasts (constexpr flash-resident array)
static constexpr const char* const WEATHER_ELEMENT_NAMES_3DAYS[] = {
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

static constexpr size_t WEATHER_ELEMENT_NAMES_3DAYS_SIZE =
    sizeof(WEATHER_ELEMENT_NAMES_3DAYS) / sizeof(WEATHER_ELEMENT_NAMES_3DAYS[0]);

// Valid weather element names for 7 days forecast (constexpr flash-resident array)
static constexpr const char* const WEATHER_ELEMENT_NAMES_7DAYS[] = {
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

static constexpr size_t WEATHER_ELEMENT_NAMES_7DAYS_SIZE =
    sizeof(WEATHER_ELEMENT_NAMES_7DAYS) / sizeof(WEATHER_ELEMENT_NAMES_7DAYS[0]);

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

// Map ElementValueKey to WeatherElementName based on Mode (constexpr flat array)
struct ModeElementMapping {
  Mode mode;
  ElementValueKey key;
  const char* element_name;
};

static constexpr ModeElementMapping MODE_ELEMENT_MAPPINGS[] = {
    // --- THREE_DAYS ---
    {Mode::THREE_DAYS, ElementValueKey::TEMPERATURE, WEATHER_ELEMENT_NAME_TEMPERATURE},
    {Mode::THREE_DAYS, ElementValueKey::DEW_POINT, WEATHER_ELEMENT_NAME_DEW_POINT},
    {Mode::THREE_DAYS, ElementValueKey::APPARENT_TEMPERATURE, WEATHER_ELEMENT_NAME_APPARENT_TEMPERATURE},
    {Mode::THREE_DAYS, ElementValueKey::COMFORT_INDEX, WEATHER_ELEMENT_NAME_COMFORT_INDEX},
    {Mode::THREE_DAYS, ElementValueKey::COMFORT_INDEX_DESCRIPTION, WEATHER_ELEMENT_NAME_COMFORT_INDEX},
    {Mode::THREE_DAYS, ElementValueKey::WEATHER, WEATHER_ELEMENT_NAME_WEATHER},
    {Mode::THREE_DAYS, ElementValueKey::WEATHER_CODE, WEATHER_ELEMENT_NAME_WEATHER},
    {Mode::THREE_DAYS, ElementValueKey::WEATHER_ICON, WEATHER_ELEMENT_NAME_WEATHER},
    {Mode::THREE_DAYS, ElementValueKey::WEATHER_DESCRIPTION, WEATHER_ELEMENT_NAME_WEATHER_DESCRIPTION},
    {Mode::THREE_DAYS, ElementValueKey::PROBABILITY_OF_PRECIPITATION, WEATHER_ELEMENT_NAME_3H_PROBABILITY_OF_PRECIPITATION},
    {Mode::THREE_DAYS, ElementValueKey::RELATIVE_HUMIDITY, WEATHER_ELEMENT_NAME_RELATIVE_HUMIDITY},
    {Mode::THREE_DAYS, ElementValueKey::WIND_DIRECTION, WEATHER_ELEMENT_NAME_WIND_DIRECTION},
    {Mode::THREE_DAYS, ElementValueKey::WIND_SPEED, WEATHER_ELEMENT_NAME_WIND_SPEED},
    {Mode::THREE_DAYS, ElementValueKey::BEAUFORT_SCALE, WEATHER_ELEMENT_NAME_WIND_SPEED},
    // --- SEVEN_DAYS ---
    {Mode::SEVEN_DAYS, ElementValueKey::TEMPERATURE, WEATHER_ELEMENT_NAME_AVG_TEMPERATURE},
    {Mode::SEVEN_DAYS, ElementValueKey::DEW_POINT, WEATHER_ELEMENT_NAME_AVG_DEW_POINT},
    {Mode::SEVEN_DAYS, ElementValueKey::RELATIVE_HUMIDITY, WEATHER_ELEMENT_NAME_AVG_RELATIVE_HUMIDITY},
    {Mode::SEVEN_DAYS, ElementValueKey::MAX_TEMPERATURE, WEATHER_ELEMENT_NAME_MAX_TEMPERATURE},
    {Mode::SEVEN_DAYS, ElementValueKey::MIN_TEMPERATURE, WEATHER_ELEMENT_NAME_MIN_TEMPERATURE},
    {Mode::SEVEN_DAYS, ElementValueKey::MAX_APPARENT_TEMPERATURE, WEATHER_ELEMENT_NAME_MAX_APPARENT_TEMPERATURE},
    {Mode::SEVEN_DAYS, ElementValueKey::MIN_APPARENT_TEMPERATURE, WEATHER_ELEMENT_NAME_MIN_APPARENT_TEMPERATURE},
    {Mode::SEVEN_DAYS, ElementValueKey::MAX_COMFORT_INDEX, WEATHER_ELEMENT_NAME_MAX_COMFORT_INDEX},
    {Mode::SEVEN_DAYS, ElementValueKey::MIN_COMFORT_INDEX, WEATHER_ELEMENT_NAME_MIN_COMFORT_INDEX},
    {Mode::SEVEN_DAYS, ElementValueKey::MIN_COMFORT_INDEX_DESCRIPTION, WEATHER_ELEMENT_NAME_MIN_COMFORT_INDEX},
    {Mode::SEVEN_DAYS, ElementValueKey::MAX_COMFORT_INDEX_DESCRIPTION, WEATHER_ELEMENT_NAME_MAX_COMFORT_INDEX},
    {Mode::SEVEN_DAYS, ElementValueKey::PROBABILITY_OF_PRECIPITATION, WEATHER_ELEMENT_NAME_12H_PROBABILITY_OF_PRECIPITATION},
    {Mode::SEVEN_DAYS, ElementValueKey::UV_INDEX, WEATHER_ELEMENT_NAME_UV_INDEX},
    {Mode::SEVEN_DAYS, ElementValueKey::WEATHER, WEATHER_ELEMENT_NAME_WEATHER},
    {Mode::SEVEN_DAYS, ElementValueKey::WEATHER_CODE, WEATHER_ELEMENT_NAME_WEATHER},
    {Mode::SEVEN_DAYS, ElementValueKey::WEATHER_ICON, WEATHER_ELEMENT_NAME_WEATHER},
    {Mode::SEVEN_DAYS, ElementValueKey::WEATHER_DESCRIPTION, WEATHER_ELEMENT_NAME_WEATHER_DESCRIPTION},
    {Mode::SEVEN_DAYS, ElementValueKey::WIND_DIRECTION, WEATHER_ELEMENT_NAME_WIND_DIRECTION},
    {Mode::SEVEN_DAYS, ElementValueKey::WIND_SPEED, WEATHER_ELEMENT_NAME_WIND_SPEED},
    {Mode::SEVEN_DAYS, ElementValueKey::BEAUFORT_SCALE, WEATHER_ELEMENT_NAME_WIND_SPEED},
    {Mode::SEVEN_DAYS, ElementValueKey::UV_EXPOSURE_LEVEL, WEATHER_ELEMENT_NAME_UV_INDEX},
};

static constexpr size_t MODE_ELEMENT_MAPPINGS_SIZE =
    sizeof(MODE_ELEMENT_MAPPINGS) / sizeof(MODE_ELEMENT_MAPPINGS[0]);

// Look up element name for a given mode and key
inline const char* find_mode_element_name(Mode mode, ElementValueKey key) {
  for (size_t i = 0; i < MODE_ELEMENT_MAPPINGS_SIZE; ++i) {
    if (MODE_ELEMENT_MAPPINGS[i].mode == mode && MODE_ELEMENT_MAPPINGS[i].key == key) {
      return MODE_ELEMENT_MAPPINGS[i].element_name;
    }
  }
  return nullptr;
}

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
  PsramTime data_time;
  PsramTime start_time_data;
  PsramTime end_time_data;
  std::vector<std::pair<ElementValueKey, PsramString>, RAMAllocator<std::pair<ElementValueKey, PsramString>>> element_values;

  Time() : element_values(RAMAllocator<std::pair<ElementValueKey, PsramString>>(RAMAllocator<std::pair<ElementValueKey, PsramString>>::NONE)) {}
  Time(const Time &o) : data_time(o.data_time), start_time_data(o.start_time_data), end_time_data(o.end_time_data) {
    for (const auto &p : o.element_values) element_values.push_back(p);
  }
  Time &operator=(const Time &o) {
    if (this == &o) return *this;
    data_time = o.data_time;
    start_time_data = o.start_time_data;
    end_time_data = o.end_time_data;
    element_values.clear();
    for (const auto &p : o.element_values) element_values.push_back(p);
    return *this;
  }
  Time(Time &&) = default;
  Time &operator=(Time &&) = default;

  std::string find_element_value(ElementValueKey key) const {
    for (const auto &p : this->element_values) {
      if (p.first == key) return p.second.to_std_string();
    }
    return std::string();
  }

  std::tm to_tm() const {
    if (this->data_time.is_valid())
      return this->data_time.to_tm();
    else if (this->start_time_data.is_valid())
      return this->start_time_data.to_tm();
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
      } else if (t.start_time_data.is_valid() && t.end_time_data.is_valid()) {
        std::tm st = t.start_time_data.to_tm();
        std::tm en = t.end_time_data.to_tm();
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
      } else if (t.start_time_data.is_valid() && t.end_time_data.is_valid()) {
        std::tm st = t.start_time_data.to_tm();
        std::tm en = t.end_time_data.to_tm();
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
        if (best->start_time_data.is_valid()) {
          std::tm st = best->start_time_data.to_tm();
          std::tm tgt = target;
          std::time_t st_epoch = std::mktime(&st);
          std::time_t tm_epoch = std::mktime(&tgt);

          if (st_epoch > tm_epoch && (st_epoch - tm_epoch) > UV_LOOKAHEAD_MINUTES * 60) {
            ESP_LOGW(TAG, "UV data fallback outside of the forecast window");
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

  // Note: Using standard types provides full compatibility while the internal
  // Time and WeatherElement structures use adaptive memory allocation for optimization

  void release_data() {
    // Use swap idiom to guarantee deallocation — shrink_to_fit() is non-binding
    // and may not actually free memory with custom allocators
    {
      std::vector<WeatherElement, RAMAllocator<WeatherElement>> empty(weather_elements.get_allocator());
      weather_elements.swap(empty);
    }  // empty destroyed here, all backing stores freed
  }

  const WeatherElement *find_weather_element(const std::string &name) const {
    for (const auto &we : weather_elements) {
      if (we.element_name == name) return &we;
    }
    return nullptr;
  }

  const WeatherElement *get_weather_element_for_key(ElementValueKey key) const {
    const char* name = find_mode_element_name(this->mode, key);
    if (!name) return nullptr;
    return this->find_weather_element(name);
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
        if (t.data_time.is_valid()) {
          datetime_str = tm_to_esptime(t.data_time.to_tm()).strftime("%Y-%m-%dT%H:%M:%S");
        } else if (t.start_time_data.is_valid() && t.end_time_data.is_valid()) {
          datetime_str = tm_to_esptime(t.start_time_data.to_tm()).strftime("%Y-%m-%dT%H:%M:%S") + " - " + tm_to_esptime(t.end_time_data.to_tm()).strftime("%Y-%m-%dT%H:%M:%S");
        }

        std::string joined_values;
        for (const auto &kv : t.element_values) {
          if (!joined_values.empty()) {
            joined_values += ", ";
          }
          joined_values += element_value_key_to_string(kv.first) + "=" + kv.second.to_std_string();
        }

        ESP_LOGI(TAG, "    Time: %s    %s", datetime_str.c_str(), joined_values.c_str());
        delay(2);
      }
    }
  }
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

  void add_weather_element(const std::string &weather_element) { this->weather_elements_.push_back(weather_element); }

  void set_weather_elements(const std::set<std::string> &weather_elements) {
    weather_elements_.assign(weather_elements.begin(), weather_elements.end());
  }

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
  void set_retry_count(V retry_count) {
    retry_count_ = retry_count;
  }

  template <typename V>
  void set_retry_delay(V retry_delay) {
    retry_delay_ = retry_delay;
  }

  void clear_data() { record_.release_data(); }

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
  void set_http_request(http_request::HttpRequestComponent *http_request) { http_request_ = http_request; }

 protected:
  bool parse_to_record(HttpStreamAdapter &stream, Record& record, uint64_t &hash_code);

  TemplatableValue<std::string> api_key_;
  TemplatableValue<std::string> city_name_;
  TemplatableValue<std::string> town_name_;
  Mode mode_;
  std::vector<std::string> weather_elements_;
  TemplatableValue<uint32_t> time_to_;
  TemplatableValue<EarlyDataClear> early_data_clear_;
  TemplatableValue<bool> fallback_to_first_element_;
  TemplatableValue<bool> retain_fetched_data_;
  TemplatableValue<uint32_t> sensor_expiry_;
  TemplatableValue<uint32_t> retry_count_;
  TemplatableValue<uint32_t> retry_delay_;
  time::RealTimeClock *rtc_{nullptr};
  http_request::HttpRequestComponent *http_request_{nullptr};

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
  bool retry_in_progress_{false};

  bool send_request_();
  void try_send_request_(uint32_t attempt);
  bool validate_config_();
  bool process_response_(HttpStreamAdapter &stream, uint64_t &hash_code);
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