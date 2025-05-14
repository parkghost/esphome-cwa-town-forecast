#include "cwa_town_forecast.h"

#include <HTTPClient.h>
#include <UrlEncode.h>
#include <sunset.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <functional>
#include <set>

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/watchdog/watchdog.h"
#include "esphome/core/helpers.h"
#include "esphome/core/time.h"

namespace esphome {
namespace cwa_town_forecast {

// Returns the setup priority for the component.
float CWATownForecast::get_setup_priority() const { return setup_priority::LATE; }

// Initializes the component.
void CWATownForecast::setup() {}

// Periodically called to update forecast data.
void CWATownForecast::update() {
  if (!validate_config_()) {
    ESP_LOGE(TAG, "Configuration validation failed");
    return;
  }

  auto now = this->rtc_->now();
  std::tm tm = now.to_c_tm();
  if (send_request_()) {
    this->status_clear_warning();

    if (this->last_success_) {
      this->last_success_->publish_state(now.strftime("%Y-%m-%d %H:%M:%S"));
    }

    this->publish_states_();

    time_t now_epoch = std::mktime(&tm);
    time_t expiry_offset = static_cast<time_t>(this->sensor_expiry_.value() / 1000);
    this->sensor_expiration_time_ = now_epoch + expiry_offset;
  } else {
    this->status_set_warning();
    this->on_error_trigger_.trigger();

    if (this->last_error_) {
      this->last_error_->publish_state(now.strftime("%Y-%m-%d %H:%M:%S"));
    }

    time_t epoch = std::mktime(&tm);
    char now_buf[20];
    std::strftime(now_buf, sizeof(now_buf), "%Y-%m-%d %H:%M:%S", &tm);
    std::tm exp_tm = *std::localtime(&this->sensor_expiration_time_);
    char exp_buf[20];
    std::strftime(exp_buf, sizeof(exp_buf), "%Y-%m-%d %H:%M:%S", &exp_tm);
    ESP_LOGV(TAG, "Current time: %s, Expiration time: %s", now_buf, exp_buf);
    if (epoch > this->sensor_expiration_time_) {
      ESP_LOGW(TAG, "Sensor data expired");
      this->publish_states_();
    }
  }

  if (!this->retain_fetched_data_.value()) {
    record_.weather_elements.clear();
  }
}

// Logs the component configuration.
void CWATownForecast::dump_config() {
  bool psram_available = false;
#ifdef USE_PSRAM
  psram_available = true;
#endif

  ESP_LOGCONFIG(TAG, "CWA Town Forecast:");
  ESP_LOGCONFIG(TAG, "  API Key: %s", api_key_.value().empty() ? "not set" : "set");
  ESP_LOGCONFIG(TAG, "  City Name: %s", city_name_.value().c_str());
  ESP_LOGCONFIG(TAG, "  Town Name: %s", town_name_.value().c_str());
  ESP_LOGCONFIG(TAG, "  Mode: %s", mode_to_string(mode_).c_str());
  ESP_LOGCONFIG(TAG, "  Weather Elements: %s", this->weather_elements_.empty() ? "not set" : "");
  for (const auto &element_name : this->weather_elements_) {
    ESP_LOGCONFIG(TAG, "    %s", element_name.c_str());
  }
  if (!time_to_.has_value()) {
    ESP_LOGCONFIG(TAG, "  Time To: not set");
  } else {
    ESP_LOGCONFIG(TAG, "  Time To: %lu hours", static_cast<unsigned long>(time_to_.value() / 1000 / 3600));
  }
  ESP_LOGCONFIG(TAG, "  Early Data Clear: %s", early_data_clear_to_string(early_data_clear_.value()).c_str());
  ESP_LOGCONFIG(TAG, "  Fallback to First Element: %s", fallback_to_first_element_.value() ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Retain Fetched Data: %s", retain_fetched_data_.value() ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Sensor Expiry: %u minutes", sensor_expiry_.value() / 1000 / 60);
  ESP_LOGCONFIG(TAG, "  Watchdog Timeout: %u ms", watchdog_timeout_.value());
  ESP_LOGCONFIG(TAG, "  HTTP Connect Timeout: %u ms", http_connect_timeout_.value());
  ESP_LOGCONFIG(TAG, "  HTTP Timeout: %u ms", http_timeout_.value());
  ESP_LOGCONFIG(TAG, "  PSRAM Available: %s", psram_available ? "true" : "false");
  LOG_UPDATE_INTERVAL(this);
}

// Validates the configuration parameters.
bool CWATownForecast::validate_config_() {
  if (api_key_.value().empty()) {
    ESP_LOGE(TAG, "API Key not set");
    return false;
  }
  if (city_name_.value().empty()) {
    ESP_LOGE(TAG, "City name not set");
    return false;
  }
  if (CITY_NAMES.find(city_name_.value()) == CITY_NAMES.end()) {
    ESP_LOGE(TAG, "Invalid city name: %s", city_name_.value().c_str());
    return false;
  }
  if (town_name_.value().empty()) {
    ESP_LOGE(TAG, "Town name not set");
    return false;
  }
  if (!this->weather_elements_.empty()) {
    Mode mode = this->mode_;
    const std::set<std::string> *valid_names;
    if (mode == Mode::THREE_DAYS) {
      valid_names = &WEATHER_ELEMENT_NAMES_3DAYS;
    } else {
      valid_names = &WEATHER_ELEMENT_NAMES_7DAYS;
    }
    for (const auto &element_name : this->weather_elements_) {
      if (valid_names->find(element_name) == valid_names->end()) {
        ESP_LOGE(TAG, "Invalid weather element for mode %s: %s", mode_to_string(mode).c_str(), element_name.c_str());
        return false;
      }
    }
  }
  return true;
}

// Sends HTTP request to fetch forecast data.
bool CWATownForecast::send_request_() {
  if (!this->rtc_->now().is_valid()) {
    ESP_LOGW(TAG, "RTC is not valid");
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    ESP_LOGW(TAG, "WiFi not connected");
    return false;
  }
  watchdog::WatchdogManager wdm(this->watchdog_timeout_.value());

  bool psram_available = false;
#ifdef USE_PSRAM
  psram_available = true;
#endif

  switch (early_data_clear_.value()) {
    case AUTO:
      if (!psram_available) {
        ESP_LOGD(TAG, "[Auto] Clear forecast data before sending request");
        this->record_.weather_elements.clear();
      }
      break;

    case ON:
      ESP_LOGD(TAG, "[On] Clear forecast data before sending request");
      this->record_.weather_elements.clear();
      break;
  }

  std::string city_name = city_name_.value();

  // Get the appropriate resource ID based on forecast mode
  Mode mode = mode_;
  std::string resource_id;
  const auto &mapping = (mode == Mode::THREE_DAYS ? CITY_NAME_TO_3D_RESOURCE_ID_MAP : CITY_NAME_TO_7D_RESOURCE_ID_MAP);
  auto it = mapping.find(city_name);
  if (it == mapping.end()) {
    ESP_LOGE(TAG, "Invalid city name: %s", city_name.c_str());
    return false;
  }
  resource_id = it->second;

  ESP_LOGD(TAG, "City name: %s, Town name: %s, Mode: %s", city_name.c_str(), town_name_.value().c_str(), mode_to_string(mode).c_str());
  ESP_LOGD(TAG, "Resource ID: %s", resource_id.c_str());
  String encoded_town_name = urlEncode(town_name_.value().c_str());
  std::string element_param;
  if (!this->weather_elements_.empty()) {
    std::vector<std::string> encoded_names;
    for (const auto &name : this->weather_elements_) {
      encoded_names.emplace_back(urlEncode(name.c_str()).c_str());
    }
    std::string joined;
    for (size_t i = 0; i < encoded_names.size(); ++i) {
      if (i > 0) joined += ",";
      joined += encoded_names[i];
    }
    element_param = "&ElementName=" + joined;
  }
  std::string time_to_param;
  if (time_to_.has_value()) {
    ESPTime now = this->rtc_->now();
    if (now.is_valid()) {
      char buffer[25];
      unsigned long seconds = time_to_.value() / 1000;
      std::tm tm_now = now.to_c_tm();
      time_t t = std::mktime(&tm_now);
      t += seconds;
      std::tm tm_new = ESPTime::from_epoch_local(t).to_c_tm();
      std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm_new);
      std::string encoded_buffer = urlEncode(buffer).c_str();
      time_to_param = "&timeTo=" + encoded_buffer;
    } else {
      ESP_LOGW(TAG, "Ignoring timeTo parameter, RTC not set");
    }
  }
  std::string url = "https://opendata.cwa.gov.tw/api/v1/rest/datastore/" + resource_id + "?Authorization=" + api_key_.value() +
                    "&format=JSON&LocationName=" + std::string(encoded_town_name.c_str()) + element_param + time_to_param;

  HTTPClient http;
  http.useHTTP10(true);
  http.setConnectTimeout(http_connect_timeout_.value());
  http.setTimeout(http_timeout_.value());
  http.addHeader("Content-Type", "application/json");
  http.begin(url.c_str());
  ESP_LOGD(TAG, "Sending query: %s", url.c_str());
  ESP_LOGD(TAG, "Before request: free heap:%u, max block:%u", ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
  App.feed_wdt();
  int http_code = http.GET();
  ESP_LOGD(TAG, "After request: free heap:%u, max block:%u", ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
  if (http_code == HTTP_CODE_OK) {
    App.feed_wdt();
    uint64_t hash_code = 0;
    bool result = this->process_response_(http.getStream(), hash_code);
    http.end();
    ESP_LOGD(TAG, "After json parse: free heap:%u, max block:%u", ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));

    if (result) {
      if (this->check_changes(hash_code)) {
        ESP_LOGD(TAG, "Data changed");
        this->on_data_change_trigger_.trigger(this->record_);
      } else {
        ESP_LOGD(TAG, "No data change detected");
      }
      return true;
    } else {
      ESP_LOGE(TAG, "Failed to parse JSON response");
      return false;
    }
  } else {
    ESP_LOGE(TAG, "HTTP request failed, code: %d", http_code);
    return false;
  }
}

// Parses ISO8601 date/time string into std::tm.
static bool parse_iso8601(const std::string &s, std::tm &tm) {
  std::memset(&tm, 0, sizeof(tm));
  int year, month, day, hour = 0, min = 0, sec = 0;
  int matched;
  if (s.find('T') != std::string::npos) {
    matched = std::sscanf(s.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &min, &sec);
    if (matched < 6) return false;
  } else {
    matched = std::sscanf(s.c_str(), "%d-%d-%d", &year, &month, &day);
    if (matched != 3) return false;
  }
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = min;
  tm.tm_sec = sec;
  return true;
}

// Processes the HTTP response and parses forecast data.
bool CWATownForecast::process_response_(Stream &stream, uint64_t &hash_code) {
  Record temp_record;

  if (!stream.find("\"success\":")) {
    ESP_LOGE(TAG, "Could not find success field");
    return false;
  }
  String success_val = stream.readStringUntil(',');
  if (success_val != "\"true\"") {
    ESP_LOGE(TAG, "API response 'success' is not true: %s", success_val.c_str());
    return false;
  }
  temp_record.mode = this->mode_;
  temp_record.weather_elements.reserve(this->mode_ == Mode::THREE_DAYS ? WEATHER_ELEMENT_NAMES_3DAYS.size() : WEATHER_ELEMENT_NAMES_7DAYS.size());
  if (!stream.find("\"LocationsName\":\"")) {
    ESP_LOGE(TAG, "Could not find LocationsName");
    return false;
  }
  temp_record.locations_name = stream.readStringUntil('"').c_str();

  if (!stream.find("\"LocationName\":\"")) {
    ESP_LOGE(TAG, "Could not find LocationName");
    return false;
  }
  temp_record.location_name = stream.readStringUntil('"').c_str();

  if (!stream.find("\"Latitude\":\"")) {
    ESP_LOGE(TAG, "Could not find Latitude");
    return false;
  }
  String lat_str = stream.readStringUntil('"');
  const char *lat_cstr = lat_str.c_str();
  char *lat_end = nullptr;
  double lat_val = std::strtod(lat_cstr, &lat_end);
  if (lat_cstr == lat_end || *lat_end != '\0') {
    ESP_LOGW(TAG, "Invalid latitude value: %s", lat_cstr);
    temp_record.latitude = NAN;
  } else {
    temp_record.latitude = lat_val;
  }

  if (!stream.find("\"Longitude\":\"")) {
    ESP_LOGE(TAG, "Could not find Longitude");
    return false;
  }
  String lon_str = stream.readStringUntil('"');
  const char *lon_cstr = lon_str.c_str();
  char *lon_end = nullptr;
  double lon_val = std::strtod(lon_cstr, &lon_end);
  if (lon_cstr == lon_end || *lon_end != '\0') {
    ESP_LOGW(TAG, "Invalid longitude value: %s", lon_cstr);
    temp_record.longitude = NAN;
  } else {
    temp_record.longitude = lon_val;
  }

  if (!stream.find("\"WeatherElement\":[")) {
    ESP_LOGE(TAG, "Could not find WeatherElement array");
    return false;
  }

  ESPTime now = this->rtc_->now();
  SunSet sun;
  double timezone_offset = static_cast<double>(now.timezone_offset()) / 60 / 60;
  sun.setPosition(temp_record.latitude, temp_record.longitude, timezone_offset);
  ESP_LOGD(TAG, "Sunset Latitude: %f, Longitude: %f, Offset: %d", temp_record.latitude, temp_record.longitude, timezone_offset);

  const size_t chunk_capacity = 1000;
  ArduinoJson::DynamicJsonDocument time_obj(chunk_capacity);
  do {
    App.feed_wdt();
    WeatherElement we;
    if (!stream.find("\"ElementName\":\"")) {
      ESP_LOGE(TAG, "Could not find ElementName");
      return false;
    }
    we.element_name = stream.readStringUntil('"').c_str();
    if (!stream.find("\"Time\":[")) {
      ESP_LOGE(TAG, "Could not find Time array for %s", we.element_name.c_str());
      return false;
    }

    // check for empty array
    int next = stream.peek();
    if (next == ']') {
      ESP_LOGW(TAG, "Empty Time array for %s, skipping element processing", we.element_name.c_str());
      stream.read();
      continue;
    }

    do {
      DeserializationError err = deserializeJson(time_obj, stream);
      if (err) {
        ESP_LOGE(TAG, "JSON parsing failed: %s", err.c_str());
        return false;
      }

      Time ts;
      if (time_obj.containsKey("DataTime")) {
        std::string tmp = time_obj["DataTime"].as<std::string>();
        ts.data_time = std::unique_ptr<std::tm>(new std::tm());
        if (!parse_iso8601(tmp, *ts.data_time)) {
          ESP_LOGE(TAG, "Could not parse DataTime: %s", tmp.c_str());
          return false;
        }
      }
      if (time_obj.containsKey("StartTime")) {
        std::string tmp = time_obj["StartTime"].as<std::string>();
        ts.start_time = std::unique_ptr<std::tm>(new std::tm());
        if (!parse_iso8601(tmp, *ts.start_time)) {
          ESP_LOGE(TAG, "Could not parse StartTime: %s", tmp.c_str());
          return false;
        }
      }
      if (time_obj.containsKey("EndTime")) {
        std::string tmp = time_obj["EndTime"].as<std::string>();
        ts.end_time = std::unique_ptr<std::tm>(new std::tm());
        if (!parse_iso8601(tmp, *ts.end_time)) {
          ESP_LOGE(TAG, "Could not parse EndTime: %s", tmp.c_str());
          return false;
        }
      }

      // Process element values
      for (JsonObject val_obj : time_obj["ElementValue"].as<JsonArray>()) {
        for (JsonPair kv : val_obj) {
          ElementValueKey evk;
          if (parse_element_value_key(kv.key().c_str(), evk)) {
            auto value = kv.value().as<std::string>();
            auto it = std::find_if(ts.element_values.begin(), ts.element_values.end(),
                                   [&](const std::pair<ElementValueKey, std::string> &p) { return p.first == evk; });
            if (it != ts.element_values.end())
              it->second = value;
            else
              ts.element_values.emplace_back(evk, value);

            // Special handling for weather codes to generate weather icons
            if (we.element_name == WEATHER_ELEMENT_NAME_WEATHER && evk == ElementValueKey::WEATHER_CODE) {
              auto it_icon = WEATHER_CODE_TO_WEATHER_ICON_NAME_MAP.find(value);
              if (it_icon != WEATHER_CODE_TO_WEATHER_ICON_NAME_MAP.end()) {
                std::string icon = it_icon->second;

                // Adjust icon based on time of day
                std::tm t = ts.to_tm();
                sun.setCurrentDate(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
                double sunrise = sun.calcSunrise();
                double sunset = sun.calcSunset();
                int sunrise_hour = static_cast<int>(sunrise / 60);
                int sunrise_minute = static_cast<int>(sunrise) % 60;
                int sunset_hour = static_cast<int>(sunset / 60);
                int sunset_minute = static_cast<int>(sunset) % 60;
                ESP_LOGV(TAG, "Date: %04d-%02d-%02d Sunrise: %02d:%02d, Sunset: %02d:%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, sunrise_hour,
                         sunrise_minute, sunset_hour, sunset_minute);

                if (t.tm_hour + 1 < sunrise_hour || t.tm_hour + 1 >= sunset_hour) {
                  if (icon == "sunny") {
                    icon = "night";
                  } else if (icon == "partly-cloudy" || icon == "cloudy") {
                    icon = "night-partly-cloudy";
                  }
                }
                ts.element_values.emplace_back(ElementValueKey::WEATHER_ICON, icon);
              } else {
                ts.element_values.emplace_back(ElementValueKey::WEATHER_ICON, "");
              }
            }
          } else {
            ESP_LOGW(TAG, "Unknown element value key: %s", kv.key().c_str());
          }
        }
        we.times.push_back(std::move(ts));
      }
    } while (stream.findUntil(",", "]"));
    temp_record.weather_elements.push_back(std::move(we));
  } while (stream.findUntil(",", "]"));

  // Determine start and end time for the entire record
  bool first_time = true;
  std::tm min_tm{};
  std::tm max_tm{};
  for (const auto &we : temp_record.weather_elements) {
    for (const auto &t : we.times) {
      std::tm candidate_tm{};
      if (t.data_time) {
        candidate_tm = *t.data_time;
      } else if (t.start_time) {
        candidate_tm = *t.start_time;
      } else {
        continue;
      }
      std::time_t cand_epoch = std::mktime(&candidate_tm);
      if (first_time || cand_epoch < std::mktime(&min_tm)) {
        min_tm = candidate_tm;
      }
      if (first_time) {
        if (t.end_time) {
          max_tm = *t.end_time;
        } else {
          max_tm = candidate_tm;
        }
      } else if (t.end_time) {
        std::time_t end_epoch = std::mktime(t.end_time.get());
        if (end_epoch > std::mktime(&max_tm)) {
          max_tm = *t.end_time;
        }
      }
      first_time = false;
    }
  }
  if (!first_time) {
    temp_record.start_time = min_tm;
    temp_record.end_time = max_tm;
  }

  // Set the updated time to current time
  if (now.is_valid()) {
    temp_record.updated_time = now.to_c_tm();
  }

  // Calculate hash code for change detection
  uint64_t new_hash = 0;
  std::hash<std::string> hasher;
  const uint64_t salt = 0x9e3779b97f4a7c15ULL;
  auto combine = [&](const std::string &s) {
    uint64_t h = hasher(s);
    new_hash ^= h + salt + (new_hash << 6) + (new_hash >> 2);
  };
  combine(temp_record.locations_name);
  combine(temp_record.location_name);
  for (const auto &we : temp_record.weather_elements) {
    combine(we.element_name);
    for (const auto &ts : we.times) {
      for (const auto &p : ts.element_values) {
        combine(element_value_key_to_string(p.first) + p.second);
      }
    }
  }

  // Swap the temporary record with the main record
  this->record_.mode = temp_record.mode;
  this->record_.locations_name = temp_record.locations_name;
  this->record_.location_name = temp_record.location_name;
  this->record_.latitude = temp_record.latitude;
  this->record_.longitude = temp_record.longitude;
  this->record_.start_time = temp_record.start_time;
  this->record_.end_time = temp_record.end_time;
  this->record_.updated_time = temp_record.updated_time;
  // Use swap for the vector to avoid allocator comparison
  this->record_.weather_elements.clear();
  this->record_.weather_elements.swap(temp_record.weather_elements);
  hash_code = new_hash;
  return true;
}

// Returns the latest forecast data record.
Record &CWATownForecast::get_data() {
  if (!this->retain_fetched_data_.value()) {
    ESP_LOGE(TAG, "Trun on retain_fetched_data option to get forecast data");
  }
  return record_;
}

// Checks if the data has changed based on hash code.
bool CWATownForecast::check_changes(uint64_t new_hash_code) {
  if (new_hash_code != this->last_hash_code_) {
    this->last_hash_code_ = new_hash_code;
    return true;
  }
  return false;
}

// Publishes the state of the sensor or text sensor.
template <typename SensorT, typename PublishValFunc, typename PublishNoMatchFunc>
void CWATownForecast::publish_state_common_(SensorT *sensor, ElementValueKey key, std::tm &target_tm, bool fallback_to_first, PublishValFunc publish_val,
                                            PublishNoMatchFunc publish_no_match) {
  if (!sensor) return;  // Skip if sensor is null

  // Find the corresponding element name for this key in the current mode
  auto mode_it = MODE_ELEMENT_NAME_MAP.find(this->record_.mode);
  if (mode_it == MODE_ELEMENT_NAME_MAP.end()) {
    ESP_LOGE(TAG, "Invalid mode: %s", mode_to_string(this->mode_).c_str());
    publish_no_match(sensor);
    return;
  }

  const auto &elem_map = mode_it->second;
  auto name_it = elem_map.find(key);
  if (name_it == elem_map.end()) {
    ESP_LOGE(TAG, "Invalid element value key: %s", element_value_key_to_string(key).c_str());
    publish_no_match(sensor);
    return;
  }

  // Get the element name and find the corresponding weather element
  std::string element_name = name_it->second;
  const WeatherElement *we = this->record_.find_weather_element(element_name);
  if (we && !we->times.empty()) {
    Time *ts = we->match_time(target_tm, key, fallback_to_first);
    if (ts) {
#if ESP_LOG_LEVEL >= ESP_LOG_VERBOSE
      if (ts->data_time)
        ESP_LOGV(TAG, "matched (%s): %s", element_name.c_str(), tm_to_esptime(*ts->data_time).strftime("%Y-%m-%d %H:%M").c_str());
      else if (ts->start_time && ts->end_time)
        ESP_LOGV(TAG, "matched (%s): %s - %s", element_name.c_str(), tm_to_esptime(*ts->start_time).strftime("%Y-%m-%d %H:%M").c_str(),
                 tm_to_esptime(*ts->end_time).strftime("%Y-%m-%d %H:%M").c_str());
#endif
      auto val = ts->find_element_value(key);
      if (!val.empty()) {
#if ESP_LOG_LEVEL >= ESP_LOG_VERBOSE
        ESP_LOGV(TAG, "%s value: %s", element_value_key_to_string(key).c_str(), val.c_str());
#endif
        publish_val(sensor, val);
        return;
      }
    }
    ESP_LOGW(TAG, "No match found for %s", element_name.c_str());
    publish_no_match(sensor);
  } else {
    ESP_LOGW(TAG, "No weather element found for %s", element_name.c_str());
    publish_no_match(sensor);
  }
}

// Publishes a numeric sensor value
void CWATownForecast::publish_sensor_state_(sensor::Sensor *sensor, ElementValueKey key, std::tm &target_tm, bool fallback_to_first) {
  publish_state_common_(
      sensor, key, target_tm, fallback_to_first,
      // Lambda for publishing numeric value
      [](sensor::Sensor *sensor, const std::string &val) {
        char *endptr = nullptr;
        float fval = std::strtof(val.c_str(), &endptr);
        if (endptr != val.c_str() && *endptr == '\0') {
          sensor->publish_state(fval);
        } else {
          ESP_LOGW(TAG, "Invalid numeric value: %s", val.c_str());
          sensor->publish_state(NAN);
        }
      },
      // Lambda for no match case
      [](sensor::Sensor *sensor) { sensor->publish_state(NAN); });
}

// Publishes a text sensor value
void CWATownForecast::publish_text_sensor_state_(text_sensor::TextSensor *sensor, ElementValueKey key, std::tm &target_tm, bool fallback_to_first) {
  publish_state_common_(
      sensor, key, target_tm, fallback_to_first,
      // Lambda for publishing text value
      [](text_sensor::TextSensor *sensor, const std::string &val) { sensor->publish_state(val); },
      // Lambda for no match case
      [](text_sensor::TextSensor *sensor) { sensor->publish_state(""); });
}

// Publishes all weather states to sensors/text sensors.
void CWATownForecast::publish_states_() {
  // Publish diagnostic sensors: city and town names
  if (this->city_sensor_) {
    this->city_sensor_->publish_state(record_.locations_name);
  }
  if (this->town_sensor_) {
    this->town_sensor_->publish_state(record_.location_name);
  }

  // Get current time for time-based data
  ESPTime now = this->rtc_->now();
  if (!now.is_valid()) {
    ESP_LOGW(TAG, "Time not set, cannot publish time-based states");
    return;
  }

  std::string now_str = now.strftime("%Y-%m-%d %H:%M:%S");
  ESP_LOGD(TAG, "Target time for state publishing: %s", now_str.c_str());

  // Publish last updated time
  if (this->last_updated_) {
    this->last_updated_->publish_state(now_str);
  }

  std::tm target_tm = now.to_c_tm();
  // Get the fallback flag for missing data
  bool fallback = this->fallback_to_first_element_.value();

  // Publish weather states based on forecast mode
  if (mode_ == Mode::THREE_DAYS) {
    // 3-day mode sensors
    if (this->temperature_) publish_sensor_state_(this->temperature_, ElementValueKey::TEMPERATURE, target_tm, fallback);
    if (this->dew_point_) publish_sensor_state_(this->dew_point_, ElementValueKey::DEW_POINT, target_tm, fallback);
    if (this->apparent_temperature_) publish_sensor_state_(this->apparent_temperature_, ElementValueKey::APPARENT_TEMPERATURE, target_tm, fallback);
    if (this->comfort_index_) publish_text_sensor_state_(this->comfort_index_, ElementValueKey::COMFORT_INDEX, target_tm, fallback);
    if (this->comfort_index_description_)
      publish_text_sensor_state_(this->comfort_index_description_, ElementValueKey::COMFORT_INDEX_DESCRIPTION, target_tm, fallback);
    if (this->relative_humidity_) publish_sensor_state_(this->relative_humidity_, ElementValueKey::RELATIVE_HUMIDITY, target_tm, fallback);
    if (this->wind_speed_) publish_sensor_state_(this->wind_speed_, ElementValueKey::WIND_SPEED, target_tm, fallback);
    if (this->probability_of_precipitation_)
      publish_sensor_state_(this->probability_of_precipitation_, ElementValueKey::PROBABILITY_OF_PRECIPITATION, target_tm, fallback);
    if (this->weather_) publish_text_sensor_state_(this->weather_, ElementValueKey::WEATHER, target_tm, fallback);
    if (this->weather_code_) publish_text_sensor_state_(this->weather_code_, ElementValueKey::WEATHER_CODE, target_tm, fallback);
    if (this->weather_description_) publish_text_sensor_state_(this->weather_description_, ElementValueKey::WEATHER_DESCRIPTION, target_tm, fallback);
    if (this->weather_icon_) publish_text_sensor_state_(this->weather_icon_, ElementValueKey::WEATHER_ICON, target_tm, fallback);
    if (this->wind_direction_) publish_text_sensor_state_(this->wind_direction_, ElementValueKey::WIND_DIRECTION, target_tm, fallback);
    if (this->beaufort_scale_) publish_text_sensor_state_(this->beaufort_scale_, ElementValueKey::BEAUFORT_SCALE, target_tm, fallback);
  } else if (mode_ == Mode::SEVEN_DAYS) {
    // 7-day mode sensors
    if (this->temperature_) publish_sensor_state_(this->temperature_, ElementValueKey::TEMPERATURE, target_tm, fallback);
    if (this->dew_point_) publish_sensor_state_(this->dew_point_, ElementValueKey::DEW_POINT, target_tm, fallback);
    if (this->relative_humidity_) publish_sensor_state_(this->relative_humidity_, ElementValueKey::RELATIVE_HUMIDITY, target_tm, fallback);
    if (this->wind_speed_) publish_sensor_state_(this->wind_speed_, ElementValueKey::WIND_SPEED, target_tm, fallback);
    if (this->beaufort_scale_) publish_text_sensor_state_(this->beaufort_scale_, ElementValueKey::BEAUFORT_SCALE, target_tm, fallback);
    if (this->probability_of_precipitation_)
      publish_sensor_state_(this->probability_of_precipitation_, ElementValueKey::PROBABILITY_OF_PRECIPITATION, target_tm, fallback);
    if (this->max_temperature_) publish_sensor_state_(this->max_temperature_, ElementValueKey::MAX_TEMPERATURE, target_tm, fallback);
    if (this->min_temperature_) publish_sensor_state_(this->min_temperature_, ElementValueKey::MIN_TEMPERATURE, target_tm, fallback);
    if (this->max_apparent_temperature_) publish_sensor_state_(this->max_apparent_temperature_, ElementValueKey::MAX_APPARENT_TEMPERATURE, target_tm, fallback);
    if (this->min_apparent_temperature_) publish_sensor_state_(this->min_apparent_temperature_, ElementValueKey::MIN_APPARENT_TEMPERATURE, target_tm, fallback);
    if (this->max_comfort_index_) publish_text_sensor_state_(this->max_comfort_index_, ElementValueKey::MAX_COMFORT_INDEX, target_tm, fallback);
    if (this->min_comfort_index_) publish_text_sensor_state_(this->min_comfort_index_, ElementValueKey::MIN_COMFORT_INDEX, target_tm, fallback);
    if (this->max_comfort_index_description_)
      publish_text_sensor_state_(this->max_comfort_index_description_, ElementValueKey::MAX_COMFORT_INDEX_DESCRIPTION, target_tm, fallback);
    if (this->min_comfort_index_description_)
      publish_text_sensor_state_(this->min_comfort_index_description_, ElementValueKey::MIN_COMFORT_INDEX_DESCRIPTION, target_tm, fallback);
    if (this->uv_index_) publish_sensor_state_(this->uv_index_, ElementValueKey::UV_INDEX, target_tm, fallback);
    if (this->uv_exposure_level_) publish_text_sensor_state_(this->uv_exposure_level_, ElementValueKey::UV_EXPOSURE_LEVEL, target_tm, fallback);
    if (this->weather_) publish_text_sensor_state_(this->weather_, ElementValueKey::WEATHER, target_tm, fallback);
    if (this->weather_code_) publish_text_sensor_state_(this->weather_code_, ElementValueKey::WEATHER_CODE, target_tm, fallback);
    if (this->weather_icon_) publish_text_sensor_state_(this->weather_icon_, ElementValueKey::WEATHER_ICON, target_tm, fallback);
    if (this->weather_description_) publish_text_sensor_state_(this->weather_description_, ElementValueKey::WEATHER_DESCRIPTION, target_tm, fallback);
    if (this->wind_direction_) publish_text_sensor_state_(this->wind_direction_, ElementValueKey::WIND_DIRECTION, target_tm, fallback);
  } else {
    ESP_LOGE(TAG, "Invalid mode in publish_states_: %s", mode_to_string(mode_).c_str());
  }
}

}  // namespace cwa_town_forecast
}  // namespace esphome
