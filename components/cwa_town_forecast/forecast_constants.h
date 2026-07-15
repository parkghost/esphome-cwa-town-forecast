#pragma once

// Flash-resident constant tables for the CWA town forecast API and their
// lookup helpers: forecast modes, weather element names, element value keys,
// mode-to-element mappings, weather-code-to-icon mapping, and city-to-resource
// ID mappings.

#include <cstddef>
#include <cstring>
#include <string>
#include <utility>

namespace esphome {
namespace cwa_town_forecast {

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

// Parse a string key to ElementValueKey enum
static inline bool parse_element_value_key(const char *key, ElementValueKey &out) {
  for (auto &p : ELEMENT_VALUE_KEY_NAMES) {
    if (strcmp(key, p.second) == 0) {
      out = p.first;
      return true;
    }
  }
  return false;
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

inline const char* find_weather_icon(const char* weather_code) {
  for (size_t i = 0; i < WEATHER_CODE_TO_WEATHER_ICON_NAME_MAP_SIZE; ++i) {
    if (strcmp(weather_code, WEATHER_CODE_TO_WEATHER_ICON_NAME_MAP[i].code) == 0) {
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

}  // namespace cwa_town_forecast
}  // namespace esphome
