#pragma once

#include <cstdint>
#include <ctime>
#include <limits>
#include <memory>
#include <set>
#include <string>
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
namespace cwa_town_forecast {

static const char *const TAG = "cwa_town_forecast";
static constexpr int DAYTIME_START_HOUR = 5;
static constexpr int DAYTIME_END_HOUR = 18;
static constexpr int UV_LOOKAHEAD_MINUTES = 90;

enum Mode {
  ThreeDays,
  SevenDays,
};

static inline std::string mode_to_string(Mode mode) {
  switch (mode) {
    case Mode::ThreeDays:
      return "ThreeDays";
    case Mode::SevenDays:
      return "SevenDays";
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

// Data structures for parsed forecast records using std types
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
    {Mode::ThreeDays,
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
    {Mode::SevenDays,
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

static const std::map<std::string, std::string> WEATHER_CODE_TO_WEATHER_ICON_NAME_MAP = {
    {"01", "mdi:weather-sunny"},
    {"02", "mdi:weather-partly-cloudy"},
    {"03", "mdi:weather-partly-cloudy"},
    {"04", "mdi:weather-partly-cloudy"},
    {"05", "mdi:weather-cloudy"},
    {"06", "mdi:weather-cloudy"},
    {"07", "mdi:weather-cloudy"},
    {"08", "mdi:weather-pouring"},
    {"09", "mdi:weather-partly-rainy"},
    {"10", "mdi:weather-partly-rainy"},
    {"11", "mdi:weather-rainy"},
    {"12", "mdi:weather-rainy"},
    {"13", "mdi:weather-rainy"},
    {"14", "mdi:weather-rainy"},
    {"15", "mdi:weather-lightning-rainy"},
    {"16", "mdi:weather-lightning-rainy"},
    {"17", "mdi:weather-lightning-rainy"},
    {"18", "mdi:weather-lightning-rainy"},
    {"19", "mdi:weather-partly-rainy"},
    {"20", "mdi:weather-partly-rainy"},
    {"21", "mdi:weather-partly-lightning"},
    {"22", "mdi:weather-partly-lightning"},
    {"23", "mdi:weather-snowy-rainy"},
    {"24", "mdi:weather-fog"},
    {"25", "mdi:weather-fog"},
    {"26", "mdi:weather-fog"},
    {"27", "mdi:weather-fog"},
    {"28", "mdi:weather-fog"},
    {"29", "mdi:weather-partly-rainy"},
    {"30", "mdi:weather-rainy"},
    {"31", "mdi:weather-rainy"},
    {"32", "mdi:weather-rainy"},
    {"33", "mdi:weather-partly-lightning"},
    {"34", "mdi:weather-lightning-rainy"},
    {"35", "mdi:weather-lightning-rainy"},
    {"36", "mdi:weather-lightning-rainy"},
    {"37", "mdi:weather-snowy-rainy"},
    {"38", "mdi:weather-rainy"},
    {"39", "mdi:weather-rainy"},
    {"41", "mdi:weather-lightning-rainy"},
    {"42", "mdi:weather-snowy"},
};

static const std::map<std::string, std::string> CITY_NAME_TO_3D_RESOURCE_ID_MAP = {
    {"宜蘭縣", "F-D0047-001"}, {"桃園市", "F-D0047-005"}, {"新竹縣", "F-D0047-009"}, {"苗栗縣", "F-D0047-013"}, {"彰化縣", "F-D0047-017"},
    {"南投縣", "F-D0047-021"}, {"雲林縣", "F-D0047-025"}, {"嘉義縣", "F-D0047-029"}, {"屏東縣", "F-D0047-033"}, {"臺東縣", "F-D0047-037"},
    {"花蓮縣", "F-D0047-041"}, {"澎湖縣", "F-D0047-045"}, {"基隆市", "F-D0047-049"}, {"新竹市", "F-D0047-053"}, {"嘉義市", "F-D0047-057"},
    {"臺北市", "F-D0047-061"}, {"高雄市", "F-D0047-065"}, {"新北市", "F-D0047-069"}, {"臺中市", "F-D0047-073"}, {"臺南市", "F-D0047-077"},
    {"連江縣", "F-D0047-081"}, {"金門縣", "F-D0047-085"},
};

static const std::map<std::string, std::string> CITY_NAME_TO_7D_RESOURCE_ID_MAP = {
    {"宜蘭縣", "F-D0047-003"}, {"桃園市", "F-D0047-007"}, {"新竹縣", "F-D0047-011"}, {"苗栗縣", "F-D0047-015"}, {"彰化縣", "F-D0047-019"},
    {"南投縣", "F-D0047-023"}, {"雲林縣", "F-D0047-027"}, {"嘉義縣", "F-D0047-031"}, {"屏東縣", "F-D0047-035"}, {"臺東縣", "F-D0047-039"},
    {"花蓮縣", "F-D0047-043"}, {"澎湖縣", "F-D0047-047"}, {"基隆市", "F-D0047-051"}, {"新竹市", "F-D0047-055"}, {"嘉義市", "F-D0047-059"},
    {"臺北市", "F-D0047-063"}, {"高雄市", "F-D0047-067"}, {"新北市", "F-D0047-071"}, {"臺中市", "F-D0047-075"}, {"臺南市", "F-D0047-079"},
    {"連江縣", "F-D0047-083"}, {"金門縣", "F-D0047-087"},
};

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
  std::vector<std::pair<ElementValueKey, std::string>> element_values;

  Time() = default;
  Time(const Time &o) {
    if (o.data_time) data_time.reset(new std::tm(*o.data_time));
    if (o.start_time) start_time.reset(new std::tm(*o.start_time));
    if (o.end_time) end_time.reset(new std::tm(*o.end_time));
    element_values = o.element_values;
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
    element_values = o.element_values;
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
inline std::pair<double, double> get_min_max_element_value(const std::vector<Time> &times, ElementValueKey key) {
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
  std::vector<Time> times;

  std::vector<Time> filter_times(const std::tm &start, const std::tm &end) const {
    std::vector<Time> result;
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
    ESP_LOGI(TAG, "  Start Time: %s", tm_to_esptime(this->start_time).strftime("%Y-%m-%dT%H:%M:%S").c_str());
    ESP_LOGI(TAG, "  End Time: %s", tm_to_esptime(this->end_time).strftime("%Y-%m-%dT%H:%M:%S").c_str());
    ESP_LOGI(TAG, "  Updated Time: %s", tm_to_esptime(this->updated_time).strftime("%Y-%m-%dT%H:%M:%S").c_str());
    for (const auto &we : this->weather_elements) {
      ESP_LOGI(TAG, "  Element: %s", we.element_name.c_str());
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

class CWATownForecast : public PollingComponent {
 public:
  Trigger<Record &> *get_on_data_change_trigger() { return &this->on_data_change_trigger_; }
  float get_setup_priority() const override;

  void setup() override;

  void update() override;

  void dump_config() override;

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

  template <typename V>
  void set_mode(V mode) {
    mode_ = mode;
  }

  void add_weather_element(const std::string &weather_element) { this->weather_elements_.insert(weather_element); }

  void set_weather_elements(const std::set<std::string> &weather_elements) { this->weather_elements_ = weather_elements; }

  template <typename V>
  void set_time_to(V time_to) {
    time_to_ = time_to;
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
  void set_fallback_to_first_element(V fallback) {
    fallback_to_first_element_ = fallback;
  }

  void set_time(time::RealTimeClock *rtc) { rtc_ = rtc; }

  const Record &get_data() const;

  void set_city_text_sensor(esphome::text_sensor::TextSensor *city) { city_sensor_ = city; }
  void set_town_text_sensor(esphome::text_sensor::TextSensor *town) { town_sensor_ = town; }
  void set_last_updated_text_sensor(text_sensor::TextSensor *sensor) { last_updated_ = sensor; }
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

 protected:
  Trigger<Record &> on_data_change_trigger_{};
  TemplatableValue<std::string> api_key_;
  TemplatableValue<std::string> city_name_;
  TemplatableValue<std::string> town_name_;
  Mode mode_;
  std::set<std::string> weather_elements_;
  TemplatableValue<uint32_t> time_to_;
  TemplatableValue<uint32_t> watchdog_timeout_;
  TemplatableValue<uint32_t> http_connect_timeout_;
  TemplatableValue<uint32_t> http_timeout_;
  TemplatableValue<bool> fallback_to_first_element_;
  time::RealTimeClock *rtc_{nullptr};

  uint64_t last_hash_code_{0};
  Record record_;

  esphome::text_sensor::TextSensor *city_sensor_{nullptr};
  esphome::text_sensor::TextSensor *town_sensor_{nullptr};
  text_sensor::TextSensor *last_updated_{nullptr};
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

  bool send_request_();
  bool validate_config_();
  bool process_response_(Stream &stream, uint64_t &hash_code);
  bool check_changes(uint64_t new_hash_code);
  void publish_states_(std::tm target_tm);
  void publish_sensor_state_(sensor::Sensor *sensor, ElementValueKey key, std::tm &target_tm, bool fallback_to_first);
  void publish_text_sensor_state_(text_sensor::TextSensor *sensor, ElementValueKey key, std::tm &target_tm, bool fallback_to_first);
  template <typename SensorT, typename PublishValFunc, typename PublishNoMatchFunc>
  void publish_state_common_(SensorT *sensor, ElementValueKey key, std::tm &target_tm, bool fallback_to_first, PublishValFunc publish_val,
                             PublishNoMatchFunc publish_no_match);
};

}  // namespace cwa_town_forecast
}  // namespace esphome
