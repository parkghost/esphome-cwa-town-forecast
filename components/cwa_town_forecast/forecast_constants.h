#pragma once

// Flash-resident constant tables for the CWA town forecast API and their
// lookup helpers: forecast modes, weather element names, element value keys,
// mode-to-element mappings, weather-code-to-icon mapping, and city-to-resource
// ID mappings.

#include <cstddef>
#include <cstdint>
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
static constexpr const char *const WEATHER_ELEMENT_NAMES_3DAYS[] = {
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
static constexpr const char *const WEATHER_ELEMENT_NAMES_7DAYS[] = {
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
    if (p.first == key)
      return p.second;
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
  const char *element_name;
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
    {Mode::THREE_DAYS, ElementValueKey::PROBABILITY_OF_PRECIPITATION,
     WEATHER_ELEMENT_NAME_3H_PROBABILITY_OF_PRECIPITATION},
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
    {Mode::SEVEN_DAYS, ElementValueKey::PROBABILITY_OF_PRECIPITATION,
     WEATHER_ELEMENT_NAME_12H_PROBABILITY_OF_PRECIPITATION},
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

static constexpr size_t MODE_ELEMENT_MAPPINGS_SIZE = sizeof(MODE_ELEMENT_MAPPINGS) / sizeof(MODE_ELEMENT_MAPPINGS[0]);

// Look up element name for a given mode and key
inline const char *find_mode_element_name(Mode mode, ElementValueKey key) {
  for (size_t i = 0; i < MODE_ELEMENT_MAPPINGS_SIZE; ++i) {
    if (MODE_ELEMENT_MAPPINGS[i].mode == mode && MODE_ELEMENT_MAPPINGS[i].key == key) {
      return MODE_ELEMENT_MAPPINGS[i].element_name;
    }
  }
  return nullptr;
}

// Icon sets available for weather-code-to-icon lookups.
enum class IconSet : uint8_t {
  WEATHER_ICONS,  // erikflowers/weather-icons (wi-*), day/night fine-grained
  MDI,            // Material Design Icons, names without the "weather-" prefix
};

// One glyph of an icon font: display name plus the UTF-8 encoded codepoint.
struct IconGlyph {
  const char *name;
  const char *unicode;
};

// MDI glyphs referenced by the weather-code table (names keep the historical
// "weather-" prefix stripped, matching the weather_icon text sensor output).
enum : uint8_t {
  MDI_SUNNY,
  MDI_PARTLY_CLOUDY,
  MDI_CLOUDY,
  MDI_POURING,
  MDI_PARTLY_RAINY,
  MDI_RAINY,
  MDI_LIGHTNING_RAINY,
  MDI_PARTLY_LIGHTNING,
  MDI_SNOWY_RAINY,
  MDI_FOG,
  MDI_SNOWY,
  MDI_NIGHT,
  MDI_NIGHT_PARTLY_CLOUDY,
};

static constexpr IconGlyph MDI_GLYPHS[] = {
    {"sunny", "\U000F0599"},
    {"partly-cloudy", "\U000F0595"},
    {"cloudy", "\U000F0590"},
    {"pouring", "\U000F0596"},
    {"partly-rainy", "\U000F0F33"},
    {"rainy", "\U000F0597"},
    {"lightning-rainy", "\U000F067E"},
    {"partly-lightning", "\U000F0F32"},
    {"snowy-rainy", "\U000F067F"},
    {"fog", "\U000F0591"},
    {"snowy", "\U000F0598"},
    {"night", "\U000F0594"},
    {"night-partly-cloudy", "\U000F0F31"},
};

// Weather Icons glyphs referenced by the weather-code table.
enum : uint8_t {
  WI_DAY_SUNNY,
  WI_DAY_SUNNY_OVERCAST,
  WI_DAY_CLOUDY,
  WI_CLOUDY,
  WI_DAY_SHOWERS,
  WI_DAY_SPRINKLE,
  WI_DAY_RAIN,
  WI_DAY_STORM_SHOWERS,
  WI_DAY_THUNDERSTORM,
  WI_DAY_SLEET,
  WI_DAY_FOG,
  WI_FOG,
  WI_DAY_SNOW,
  WI_NIGHT_CLEAR,
  WI_NIGHT_ALT_PARTLY_CLOUDY,
  WI_NIGHT_ALT_CLOUDY,
  WI_NIGHT_ALT_SHOWERS,
  WI_NIGHT_ALT_SPRINKLE,
  WI_NIGHT_ALT_RAIN,
  WI_NIGHT_ALT_STORM_SHOWERS,
  WI_NIGHT_ALT_THUNDERSTORM,
  WI_NIGHT_ALT_SLEET,
  WI_NIGHT_FOG,
  WI_NIGHT_ALT_SNOW,
};

static constexpr IconGlyph WI_GLYPHS[] = {
    {"wi-day-sunny", "\uF00D"},
    {"wi-day-sunny-overcast", "\uF00C"},
    {"wi-day-cloudy", "\uF002"},
    {"wi-cloudy", "\uF013"},
    {"wi-day-showers", "\uF009"},
    {"wi-day-sprinkle", "\uF00B"},
    {"wi-day-rain", "\uF008"},
    {"wi-day-storm-showers", "\uF00E"},
    {"wi-day-thunderstorm", "\uF010"},
    {"wi-day-sleet", "\uF0B2"},
    {"wi-day-fog", "\uF003"},
    {"wi-fog", "\uF014"},
    {"wi-day-snow", "\uF00A"},
    {"wi-night-clear", "\uF02E"},
    {"wi-night-alt-partly-cloudy", "\uF081"},
    {"wi-night-alt-cloudy", "\uF086"},
    {"wi-night-alt-showers", "\uF029"},
    {"wi-night-alt-sprinkle", "\uF02B"},
    {"wi-night-alt-rain", "\uF028"},
    {"wi-night-alt-storm-showers", "\uF02C"},
    {"wi-night-alt-thunderstorm", "\uF02D"},
    {"wi-night-alt-sleet", "\uF0B4"},
    {"wi-night-fog", "\uF04A"},
    {"wi-night-alt-snow", "\uF02A"},
};

// Weather-condition category bits derived from the CWA description of each
// weather code (rain/thunder/fog/snow keywords). Codes 01-07 (clear/cloudy)
// carry no flags.
enum : uint8_t {
  WEATHER_FLAG_RAIN = 1 << 0,
  WEATHER_FLAG_THUNDER = 1 << 1,
  WEATHER_FLAG_FOG = 1 << 2,
  WEATHER_FLAG_SNOW = 1 << 3,
};

// Per-CWA-weather-code icon mapping. Indices point into MDI_GLYPHS/WI_GLYPHS.
struct WeatherCodeIcon {
  const char *code;
  uint8_t mdi_day;
  uint8_t mdi_night;
  uint8_t wi_day;
  uint8_t wi_night;
  uint8_t flags;  // WEATHER_FLAG_* bits
};

static constexpr WeatherCodeIcon WEATHER_CODE_ICON_MAP[] = {
    {"01", MDI_SUNNY, MDI_NIGHT, WI_DAY_SUNNY, WI_NIGHT_CLEAR, 0},  // 晴天
    {"02", MDI_PARTLY_CLOUDY, MDI_NIGHT_PARTLY_CLOUDY, WI_DAY_SUNNY_OVERCAST, WI_NIGHT_ALT_PARTLY_CLOUDY,
     0},                                                                                        // 晴時多雲
    {"03", MDI_PARTLY_CLOUDY, MDI_NIGHT_PARTLY_CLOUDY, WI_DAY_CLOUDY, WI_NIGHT_ALT_CLOUDY, 0},  // 多雲時晴
    {"04", MDI_PARTLY_CLOUDY, MDI_NIGHT_PARTLY_CLOUDY, WI_DAY_CLOUDY, WI_NIGHT_ALT_CLOUDY, 0},  // 多雲
    {"05", MDI_CLOUDY, MDI_NIGHT_PARTLY_CLOUDY, WI_CLOUDY, WI_CLOUDY, 0},                       // 多雲時陰
    {"06", MDI_CLOUDY, MDI_NIGHT_PARTLY_CLOUDY, WI_CLOUDY, WI_CLOUDY, 0},                       // 陰時多雲
    {"07", MDI_CLOUDY, MDI_NIGHT_PARTLY_CLOUDY, WI_CLOUDY, WI_CLOUDY, 0},                       // 陰天
    {"08", MDI_POURING, MDI_POURING, WI_DAY_SHOWERS, WI_NIGHT_ALT_SHOWERS, WEATHER_FLAG_RAIN},  // 多雲陣雨
    {"09", MDI_PARTLY_RAINY, MDI_PARTLY_RAINY, WI_DAY_SPRINKLE, WI_NIGHT_ALT_SPRINKLE,
     WEATHER_FLAG_RAIN},  // 多雲時陰短暫雨
    {"10", MDI_PARTLY_RAINY, MDI_PARTLY_RAINY, WI_DAY_SPRINKLE, WI_NIGHT_ALT_SPRINKLE,
     WEATHER_FLAG_RAIN},                                                              // 陰時多雲短暫雨
    {"11", MDI_RAINY, MDI_RAINY, WI_DAY_RAIN, WI_NIGHT_ALT_RAIN, WEATHER_FLAG_RAIN},  // 雨天
    {"12", MDI_RAINY, MDI_RAINY, WI_DAY_RAIN, WI_NIGHT_ALT_RAIN, WEATHER_FLAG_RAIN},  // 多雲時陰有雨
    {"13", MDI_RAINY, MDI_RAINY, WI_DAY_RAIN, WI_NIGHT_ALT_RAIN, WEATHER_FLAG_RAIN},  // 陰時多雲有雨
    {"14", MDI_RAINY, MDI_RAINY, WI_DAY_RAIN, WI_NIGHT_ALT_RAIN, WEATHER_FLAG_RAIN},  // 陰有雨
    {"15", MDI_LIGHTNING_RAINY, MDI_LIGHTNING_RAINY, WI_DAY_STORM_SHOWERS, WI_NIGHT_ALT_STORM_SHOWERS,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_THUNDER},  // 多雲陣雨或雷雨
    {"16", MDI_LIGHTNING_RAINY, MDI_LIGHTNING_RAINY, WI_DAY_STORM_SHOWERS, WI_NIGHT_ALT_STORM_SHOWERS,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_THUNDER},  // 多雲時陰陣雨或雷雨
    {"17", MDI_LIGHTNING_RAINY, MDI_LIGHTNING_RAINY, WI_DAY_THUNDERSTORM, WI_NIGHT_ALT_THUNDERSTORM,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_THUNDER},  // 陰時多雲有雷陣雨
    {"18", MDI_LIGHTNING_RAINY, MDI_LIGHTNING_RAINY, WI_DAY_THUNDERSTORM, WI_NIGHT_ALT_THUNDERSTORM,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_THUNDER},  // 陰有陣雨或雷雨
    {"19", MDI_PARTLY_RAINY, MDI_PARTLY_RAINY, WI_DAY_SHOWERS, WI_NIGHT_ALT_SHOWERS,
     WEATHER_FLAG_RAIN},  // 晴午後多雲局部雨
    {"20", MDI_PARTLY_RAINY, MDI_PARTLY_RAINY, WI_DAY_SHOWERS, WI_NIGHT_ALT_SHOWERS,
     WEATHER_FLAG_RAIN},  // 多雲午後局部雨
    {"21", MDI_PARTLY_LIGHTNING, MDI_PARTLY_LIGHTNING, WI_DAY_STORM_SHOWERS, WI_NIGHT_ALT_STORM_SHOWERS,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_THUNDER},  // 晴午後多雲陣雨或雷雨
    {"22", MDI_PARTLY_LIGHTNING, MDI_PARTLY_LIGHTNING, WI_DAY_STORM_SHOWERS, WI_NIGHT_ALT_STORM_SHOWERS,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_THUNDER},  // 多雲午後局部陣雨或雷雨
    {"23", MDI_SNOWY_RAINY, MDI_SNOWY_RAINY, WI_DAY_SLEET, WI_NIGHT_ALT_SLEET,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_SNOW},                               // 短暫雨或雪
    {"24", MDI_FOG, MDI_FOG, WI_DAY_FOG, WI_NIGHT_FOG, WEATHER_FLAG_FOG},  // 晴有霧
    {"25", MDI_FOG, MDI_FOG, WI_DAY_FOG, WI_NIGHT_FOG, WEATHER_FLAG_FOG},  // 晴時多雲有霧
    {"26", MDI_FOG, MDI_FOG, WI_DAY_FOG, WI_NIGHT_FOG, WEATHER_FLAG_FOG},  // 多雲時晴有霧
    {"27", MDI_FOG, MDI_FOG, WI_DAY_FOG, WI_NIGHT_FOG, WEATHER_FLAG_FOG},  // 多雲有霧
    {"28", MDI_FOG, MDI_FOG, WI_FOG, WI_FOG, WEATHER_FLAG_FOG},            // 陰有霧
    {"29", MDI_PARTLY_RAINY, MDI_PARTLY_RAINY, WI_DAY_SHOWERS, WI_NIGHT_ALT_SHOWERS, WEATHER_FLAG_RAIN},  // 多雲局部雨
    {"30", MDI_RAINY, MDI_RAINY, WI_DAY_RAIN, WI_NIGHT_ALT_RAIN, WEATHER_FLAG_RAIN},  // 多雲時陰局部雨
    {"31", MDI_RAINY, MDI_RAINY, WI_DAY_RAIN, WI_NIGHT_ALT_RAIN,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_FOG},  // 多雲有霧有局部雨
    {"32", MDI_RAINY, MDI_RAINY, WI_DAY_RAIN, WI_NIGHT_ALT_RAIN,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_FOG},  // 多雲時陰有霧有局部雨
    {"33", MDI_PARTLY_LIGHTNING, MDI_PARTLY_LIGHTNING, WI_DAY_STORM_SHOWERS, WI_NIGHT_ALT_STORM_SHOWERS,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_THUNDER},  // 多雲局部陣雨或雷雨
    {"34", MDI_LIGHTNING_RAINY, MDI_LIGHTNING_RAINY, WI_DAY_THUNDERSTORM, WI_NIGHT_ALT_THUNDERSTORM,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_THUNDER},  // 多雲時陰局部陣雨或雷雨
    {"35", MDI_LIGHTNING_RAINY, MDI_LIGHTNING_RAINY, WI_DAY_THUNDERSTORM, WI_NIGHT_ALT_THUNDERSTORM,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_THUNDER | WEATHER_FLAG_FOG},  // 多雲有陣雨或雷雨有霧
    {"36", MDI_LIGHTNING_RAINY, MDI_LIGHTNING_RAINY, WI_DAY_THUNDERSTORM, WI_NIGHT_ALT_THUNDERSTORM,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_THUNDER | WEATHER_FLAG_FOG},  // 多雲時陰有陣雨或雷雨有霧
    {"37", MDI_SNOWY_RAINY, MDI_SNOWY_RAINY, WI_DAY_SLEET, WI_NIGHT_ALT_SLEET,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_SNOW | WEATHER_FLAG_FOG},  // 多雲局部雨或雪有霧
    {"38", MDI_RAINY, MDI_RAINY, WI_DAY_SHOWERS, WI_NIGHT_ALT_SHOWERS,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_FOG},  // 短暫陣雨有霧
    {"39", MDI_RAINY, MDI_RAINY, WI_DAY_RAIN, WI_NIGHT_ALT_RAIN, WEATHER_FLAG_RAIN | WEATHER_FLAG_FOG},  // 有雨有霧
    {"41", MDI_LIGHTNING_RAINY, MDI_LIGHTNING_RAINY, WI_DAY_STORM_SHOWERS, WI_NIGHT_ALT_STORM_SHOWERS,
     WEATHER_FLAG_RAIN | WEATHER_FLAG_THUNDER | WEATHER_FLAG_FOG},                    // 短暫陣雨或雷雨有霧
    {"42", MDI_SNOWY, MDI_SNOWY, WI_DAY_SNOW, WI_NIGHT_ALT_SNOW, WEATHER_FLAG_SNOW},  // 下雪
};

static constexpr size_t WEATHER_CODE_ICON_MAP_SIZE = sizeof(WEATHER_CODE_ICON_MAP) / sizeof(WEATHER_CODE_ICON_MAP[0]);

inline const WeatherCodeIcon *find_weather_code_icon_entry(const char *weather_code) {
  if (weather_code == nullptr)
    return nullptr;
  for (size_t i = 0; i < WEATHER_CODE_ICON_MAP_SIZE; ++i) {
    if (strcmp(weather_code, WEATHER_CODE_ICON_MAP[i].code) == 0) {
      return &WEATHER_CODE_ICON_MAP[i];
    }
  }
  return nullptr;
}

inline const IconGlyph *find_weather_icon_glyph(const char *weather_code, bool is_day, IconSet set) {
  const WeatherCodeIcon *entry = find_weather_code_icon_entry(weather_code);
  if (entry == nullptr)
    return nullptr;
  if (set == IconSet::MDI) {
    return &MDI_GLYPHS[is_day ? entry->mdi_day : entry->mdi_night];
  }
  return &WI_GLYPHS[is_day ? entry->wi_day : entry->wi_night];
}

// Icon name for a CWA weather code, e.g. "wi-day-sunny" (WEATHER_ICONS) or
// "sunny" (MDI). Returns "" for unknown codes.
inline const char *find_weather_icon_name(const char *weather_code, bool is_day, IconSet set = IconSet::WEATHER_ICONS) {
  const IconGlyph *glyph = find_weather_icon_glyph(weather_code, is_day, set);
  return glyph ? glyph->name : "";
}

// UTF-8 encoded font glyph for a CWA weather code, ready for display printf.
// Returns "" for unknown codes.
inline const char *find_weather_icon_unicode(const char *weather_code, bool is_day,
                                             IconSet set = IconSet::WEATHER_ICONS) {
  const IconGlyph *glyph = find_weather_icon_glyph(weather_code, is_day, set);
  return glyph ? glyph->unicode : "";
}

// WEATHER_FLAG_* bits for a CWA weather code; 0 for clear/cloudy or unknown.
inline uint8_t weather_code_flags(const char *weather_code) {
  const WeatherCodeIcon *entry = find_weather_code_icon_entry(weather_code);
  return entry ? entry->flags : 0;
}

// True for weather worth emphasizing on displays (rain, thunder, fog, snow);
// false for clear/cloudy codes and unknown codes.
inline bool weather_code_highlight(const char *weather_code) { return weather_code_flags(weather_code) != 0; }

struct CityResourcePair {
  const char *city;
  const char *resource_id;
};

static constexpr CityResourcePair CITY_NAME_TO_3D_RESOURCE_ID_MAP[] = {
    {"宜蘭縣", "F-D0047-001"}, {"桃園市", "F-D0047-005"}, {"新竹縣", "F-D0047-009"}, {"苗栗縣", "F-D0047-013"},
    {"彰化縣", "F-D0047-017"}, {"南投縣", "F-D0047-021"}, {"雲林縣", "F-D0047-025"}, {"嘉義縣", "F-D0047-029"},
    {"屏東縣", "F-D0047-033"}, {"臺東縣", "F-D0047-037"}, {"花蓮縣", "F-D0047-041"}, {"澎湖縣", "F-D0047-045"},
    {"基隆市", "F-D0047-049"}, {"新竹市", "F-D0047-053"}, {"嘉義市", "F-D0047-057"}, {"臺北市", "F-D0047-061"},
    {"高雄市", "F-D0047-065"}, {"新北市", "F-D0047-069"}, {"臺中市", "F-D0047-073"}, {"臺南市", "F-D0047-077"},
    {"連江縣", "F-D0047-081"}, {"金門縣", "F-D0047-085"},
};

static constexpr size_t CITY_NAME_TO_3D_RESOURCE_ID_MAP_SIZE =
    sizeof(CITY_NAME_TO_3D_RESOURCE_ID_MAP) / sizeof(CITY_NAME_TO_3D_RESOURCE_ID_MAP[0]);

static constexpr CityResourcePair CITY_NAME_TO_7D_RESOURCE_ID_MAP[] = {
    {"宜蘭縣", "F-D0047-003"}, {"桃園市", "F-D0047-007"}, {"新竹縣", "F-D0047-011"}, {"苗栗縣", "F-D0047-015"},
    {"彰化縣", "F-D0047-019"}, {"南投縣", "F-D0047-023"}, {"雲林縣", "F-D0047-027"}, {"嘉義縣", "F-D0047-031"},
    {"屏東縣", "F-D0047-035"}, {"臺東縣", "F-D0047-039"}, {"花蓮縣", "F-D0047-043"}, {"澎湖縣", "F-D0047-047"},
    {"基隆市", "F-D0047-051"}, {"新竹市", "F-D0047-055"}, {"嘉義市", "F-D0047-059"}, {"臺北市", "F-D0047-063"},
    {"高雄市", "F-D0047-067"}, {"新北市", "F-D0047-071"}, {"臺中市", "F-D0047-075"}, {"臺南市", "F-D0047-079"},
    {"連江縣", "F-D0047-083"}, {"金門縣", "F-D0047-087"},
};

static constexpr size_t CITY_NAME_TO_7D_RESOURCE_ID_MAP_SIZE =
    sizeof(CITY_NAME_TO_7D_RESOURCE_ID_MAP) / sizeof(CITY_NAME_TO_7D_RESOURCE_ID_MAP[0]);

inline const char *find_city_resource_id(const std::string &city_name, bool is_7_days) {
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
