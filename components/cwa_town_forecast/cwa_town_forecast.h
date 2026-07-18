#pragma once

#include <cstdint>
#include <cstring>
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
#include "forecast_constants.h"
#include "psram_allocator.h"
#include "string_pool.h"
#include "time_field.h"
#include "http_stream_adapter.h"
#include "url_encode.h"

#ifdef USE_PSRAM
#define CWA_PSRAM_AVAILABLE() esp_psram_is_initialized()
#else
#define CWA_PSRAM_AVAILABLE() false
#endif

namespace esphome {
namespace cwa_town_forecast {

static constexpr const char *const TAG = "cwa_town_forecast";

static constexpr int UV_LOOKAHEAD_MINUTES = 90;

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

// Convert a std::tm struct (local time) to ESPTime.
static inline ESPTime tm_to_esptime(const std::tm tm) {
  ESPTime esp_time{};
  esp_time.second = tm.tm_sec;
  esp_time.minute = tm.tm_min;
  esp_time.hour = tm.tm_hour;
  esp_time.day_of_month = tm.tm_mday;
  esp_time.month = tm.tm_mon + 1;
  esp_time.year = tm.tm_year + 1900;
  esp_time.day_of_week = tm.tm_wday == 0 ? 1 : tm.tm_wday + 1;
  esp_time.day_of_year = tm.tm_yday + 1;
  esp_time.is_dst = tm.tm_isdst > 0;
  esp_time.recalc_timestamp_local();
  return esp_time;
}

// One element value: the key plus a 16-bit offset into the record's
// StringPool where the value text lives.
struct ElementValueEntry {
  uint8_t key;      // ElementValueKey
  uint16_t offset;  // into the owning Record's StringPool
};

// Fixed-capacity inline storage for a time slot's element values. Real CWA
// payloads carry at most 2 values per slot, plus the synthesized WeatherIcon
// (verified against resources/ example responses), so capacity 3 covers all
// known data with zero heap allocation; the parser drops overflow with a
// warning. Provides the read interface of a sequence container so range-for
// iteration keeps working.
class ElementValueArray {
 public:
  using value_type = ElementValueEntry;
  static constexpr size_t CAPACITY = 3;

  value_type *begin() { return items_; }
  value_type *end() { return items_ + count_; }
  const value_type *begin() const { return items_; }
  const value_type *end() const { return items_ + count_; }
  size_t size() const { return count_; }
  bool empty() const { return count_ == 0; }

  bool emplace_back(ElementValueKey key, uint16_t offset) {
    if (count_ >= CAPACITY)
      return false;
    items_[count_].key = static_cast<uint8_t>(key);
    items_[count_].offset = offset;
    ++count_;
    return true;
  }

  void clear() { count_ = 0; }

 private:
  value_type items_[CAPACITY]{};
  uint8_t count_{0};
};

struct Time {
  TimeField data_time;
  TimeField start_time_data;
  TimeField end_time_data;
  ElementValueArray element_values;
  // Resolves element_values offsets. Shared ownership: a Time copied out of
  // the Record (e.g. a filter_times() result) keeps the pool alive on its
  // own, so stale copies read their original values instead of dangling.
  std::shared_ptr<const StringPool> string_pool;

  std::string find_element_value(ElementValueKey key) const {
    for (const auto &p : this->element_values) {
      if (p.key == static_cast<uint8_t>(key))
        return this->string_pool ? std::string(this->string_pool->get(p.offset)) : std::string();
    }
    return std::string();
  }

  // The field that represents this slot: instantaneous DataTime when present,
  // otherwise the interval's StartTime (possibly invalid when neither is set)
  const TimeField &primary_field() const {
    return this->data_time.is_valid() ? this->data_time : this->start_time_data;
  }

  std::tm to_tm() const { return this->primary_field().to_tm(); }

  // Epoch equivalent of to_tm(); -1 mirrors mktime()'s failure value when no
  // time field is set.
  time_t to_epoch() const {
    const TimeField &f = this->primary_field();
    return f.is_valid() ? f.epoch() : static_cast<time_t>(-1);
  }

  ESPTime to_esptime() const { return tm_to_esptime(this->to_tm()); }
};

// Utility function to get min and max value for a given ElementValueKey in a vector of Time
template<typename Alloc>
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
        if (num < min_val)
          min_val = num;
        if (num > max_val)
          max_val = num;
      }
    }
  }
  return {min_val, max_val};
}

struct WeatherElement {
  std::string element_name;
  std::vector<Time, PsramAllocator<Time>> times;

  std::vector<Time, PsramAllocator<Time>> filter_times(const std::tm &start, const std::tm &end) const {
    std::vector<Time, PsramAllocator<Time>> result;
    std::time_t start_epoch = TimeField::to_wall_epoch(start);
    std::time_t end_epoch = TimeField::to_wall_epoch(end);
    for (const auto &t : this->times) {
      if (t.data_time) {
        std::time_t dt_epoch = t.data_time.epoch();
        if (dt_epoch >= start_epoch && dt_epoch < end_epoch) {
          result.push_back(t);
        }
      } else if (t.start_time_data.is_valid() && t.end_time_data.is_valid()) {
        // check if interval overlaps [start, end)
        if (t.end_time_data.epoch() > start_epoch && t.start_time_data.epoch() < end_epoch) {
          result.push_back(t);
        }
      }
    }
    return result;
  }

  Time *find_closest_time(const std::tm &target) const {
    if (this->times.empty())
      return nullptr;

    size_t closest_idx = 0;
    std::time_t tgt_epoch = TimeField::to_wall_epoch(target);
    std::time_t min_diff = std::numeric_limits<std::time_t>::max();
    for (size_t i = 0; i < this->times.size(); ++i) {
      std::time_t t_epoch = this->times[i].to_epoch();
      std::time_t diff = (t_epoch > tgt_epoch) ? t_epoch - tgt_epoch : tgt_epoch - t_epoch;
      if (diff < min_diff) {
        min_diff = diff;
        closest_idx = i;
      }
    }
    return const_cast<Time *>(&this->times[closest_idx]);
  }

  Time *match_time(const std::tm &target, ElementValueKey key, bool fallback_to_first_element) const {
    if (this->times.empty())
      return nullptr;

    Time *best = nullptr;
    std::time_t tgt_epoch = TimeField::to_wall_epoch(target);
    for (const auto &t : this->times) {
      if (t.data_time) {
        if (t.data_time.epoch() <= tgt_epoch) {
          best = const_cast<Time *>(&t);
        }
      } else if (t.start_time_data.is_valid() && t.end_time_data.is_valid()) {
        if (tgt_epoch >= t.start_time_data.epoch() && tgt_epoch < t.end_time_data.epoch()) {
          return const_cast<Time *>(&t);
        }
      }
    }
    if (best == nullptr && fallback_to_first_element) {
      best = const_cast<Time *>(&this->times[0]);
      if (key == ElementValueKey::UV_EXPOSURE_LEVEL || key == ElementValueKey::UV_INDEX) {
        if (best->start_time_data.is_valid()) {
          std::time_t st_epoch = best->start_time_data.epoch();
          if (st_epoch > tgt_epoch && (st_epoch - tgt_epoch) > UV_LOOKAHEAD_MINUTES * 60) {
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

// Result of Record::find_weather_icon(): everything a display lambda needs to
// render one forecast slot's weather icon in a single lookup.
struct WeatherIconInfo {
  char code[3];         // CWA WeatherCode ("01".."42"); "" when the slot has none
  const char *name;     // icon name, e.g. "wi-day-sunny"; "" when not mapped
  const char *unicode;  // UTF-8 font glyph ready for printf; "" when not mapped
  uint8_t flags;        // WEATHER_FLAG_* bits of the code
  bool valid;           // slot had a WeatherCode with an icon mapping

  bool is_rainy() const { return (this->flags & WEATHER_FLAG_RAIN) != 0; }
  bool is_lightning() const { return (this->flags & WEATHER_FLAG_THUNDER) != 0; }
  bool is_foggy() const { return (this->flags & WEATHER_FLAG_FOG) != 0; }
  bool is_snowy() const { return (this->flags & WEATHER_FLAG_SNOW) != 0; }
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
  // Hours east of UTC, captured from the RTC when the record was parsed; used
  // for sunrise/sunset calculation. CWA data is Taiwan-only, hence the default.
  double timezone_offset{8.0};
  std::vector<WeatherElement, PsramAllocator<WeatherElement>> weather_elements;
  // Deduplicated value storage referenced by every Time's element_values.
  // Shared with the Times so the pool outlives any Time copied out of this
  // Record; heap address stays stable across Record moves.
  std::shared_ptr<StringPool> string_pool;

  // Note: Using standard types provides full compatibility while the internal
  // Time and WeatherElement structures use adaptive memory allocation for optimization

  void release_data() {
    // Use swap idiom to guarantee deallocation — shrink_to_fit() is non-binding
    // and may not actually free memory with custom allocators
    {
      std::vector<WeatherElement, PsramAllocator<WeatherElement>> empty(weather_elements.get_allocator());
      weather_elements.swap(empty);
    }  // empty destroyed here, all backing stores freed
    string_pool.reset();
  }

  const WeatherElement *find_weather_element(const std::string &name) const {
    for (const auto &we : weather_elements) {
      if (we.element_name == name)
        return &we;
    }
    return nullptr;
  }

  const WeatherElement *get_weather_element_for_key(ElementValueKey key) const {
    const char *name = find_mode_element_name(this->mode, key);
    if (!name)
      return nullptr;
    return this->find_weather_element(name);
  }

  const std::string find_value(ElementValueKey key, bool fallback_to_first_element, std::tm tm) const {
    return find_value(key, fallback_to_first_element, tm, std::string("-"));
  }

  const std::string find_value(ElementValueKey key, bool fallback_to_first_element, std::tm tm,
                               std::string default_value) const {
    const WeatherElement *we = this->get_weather_element_for_key(key);
    if (!we)
      return default_value;
    if (Time *ts = we->match_time(tm, key, fallback_to_first_element)) {
      auto val = ts->find_element_value(key);
      return val.empty() ? default_value : val;
    }
    return default_value;
  }

  // True when t falls between sunrise and sunset at this record's location
  // (hour granularity, matching the WEATHER_ICON day/night selection).
  bool is_daytime(const std::tm &t) const;

  // One-stop weather icon lookup for the slot matching tm: resolves the
  // WeatherCode, picks the day or night glyph via is_daytime(tm), and returns
  // name/unicode/condition flags together.
  WeatherIconInfo find_weather_icon(bool fallback_to_first_element, const std::tm &tm,
                                    IconSet set = IconSet::WEATHER_ICONS) const;

  const std::pair<double, double> find_min_max_values(ElementValueKey key, std::tm &start, std::tm &end) const {
    const WeatherElement *we = this->get_weather_element_for_key(key);
    if (!we)
      return std::make_pair(0.0, 0.0);
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
          datetime_str = tm_to_esptime(t.start_time_data.to_tm()).strftime("%Y-%m-%dT%H:%M:%S") + " - " +
                         tm_to_esptime(t.end_time_data.to_tm()).strftime("%Y-%m-%dT%H:%M:%S");
        }

        std::string joined_values;
        for (const auto &kv : t.element_values) {
          if (!joined_values.empty()) {
            joined_values += ", ";
          }
          joined_values += element_value_key_to_string(static_cast<ElementValueKey>(kv.key)) + "=" +
                           (this->string_pool ? this->string_pool->get(kv.offset) : "");
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

  template<typename V> void set_api_key(V key) { api_key_ = key; }

  template<typename V> void set_city_name(V city_name) { city_name_ = city_name; }

  template<typename V> void set_town_name(V town_name) { town_name_ = town_name; }

  void add_weather_element(const std::string &weather_element) { this->weather_elements_.push_back(weather_element); }

  void set_weather_elements(const std::set<std::string> &weather_elements) {
    weather_elements_.assign(weather_elements.begin(), weather_elements.end());
  }

  template<typename V> void set_time_to(V time_to) { time_to_ = time_to; }

  template<typename V> void set_sensor_expiry(V expiry) { sensor_expiry_ = expiry; }

  template<typename V> void set_fallback_to_first_element(V fallback) { fallback_to_first_element_ = fallback; }

  template<typename V> void set_retain_fetched_data(V retain) { retain_fetched_data_ = retain; }

  template<typename V> void set_early_data_clear(V early_data_clear) { early_data_clear_ = early_data_clear; }

  template<typename V> void set_retry_count(V retry_count) { retry_count_ = retry_count; }

  template<typename V> void set_retry_delay(V retry_delay) { retry_delay_ = retry_delay; }

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
  void set_comfort_index_description_text_sensor(text_sensor::TextSensor *sensor) {
    comfort_index_description_ = sensor;
  }
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
  void set_max_comfort_index_description_text_sensor(text_sensor::TextSensor *sensor) {
    max_comfort_index_description_ = sensor;
  }
  void set_min_comfort_index_description_text_sensor(text_sensor::TextSensor *sensor) {
    min_comfort_index_description_ = sensor;
  }
  void set_uv_index_sensor(sensor::Sensor *sensor) { uv_index_ = sensor; }
  void set_uv_exposure_level_text_sensor(text_sensor::TextSensor *sensor) { uv_exposure_level_ = sensor; }
  Trigger<Record &> *get_on_data_change_trigger() { return &this->on_data_change_trigger_; }
  Trigger<> *get_on_error_trigger() { return &this->on_error_trigger_; }
  void set_http_request(http_request::HttpRequestComponent *http_request) { http_request_ = http_request; }

 protected:
  bool parse_to_record(HttpStreamAdapter &stream, Record &record, uint64_t &hash_code);

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
  void publish_text_sensor_state_(text_sensor::TextSensor *sensor, ElementValueKey key, std::tm &target_tm,
                                  bool fallback_to_first);
  template<typename SensorT, typename PublishValFunc, typename PublishNoMatchFunc>
  void publish_state_common_(SensorT *sensor, ElementValueKey key, std::tm &target_tm, bool fallback_to_first,
                             PublishValFunc publish_val, PublishNoMatchFunc publish_no_match);
};

}  // namespace cwa_town_forecast
}  // namespace esphome
