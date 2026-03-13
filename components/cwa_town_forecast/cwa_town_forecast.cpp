#include "cwa_town_forecast.h"

#include <sunset.h>

#include "url_encode.h"
#include "http_stream_adapter.h"

#include <esp_random.h>
#include <esp_heap_caps.h>
#include <esp_system.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <functional>

#include "esphome/components/network/util.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/helpers.h"
#include "esphome/core/time.h"

namespace esphome {
namespace cwa_town_forecast {

// Returns the setup priority for the component.
float CWATownForecast::get_setup_priority() const { return setup_priority::LATE; }

// Initializes the component.
void CWATownForecast::setup() {
  if (CWA_PSRAM_AVAILABLE()) {
    ESP_LOGI(TAG, "PSRAM detected (%zu bytes), using extended memory allocation", heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
    this->record_.weather_elements.reserve(32);
    ESP_LOGV(TAG, "Using extended buffers for PSRAM");
  } else {
    ESP_LOGI(TAG, "No PSRAM detected, using conservative memory allocation");
    this->record_.weather_elements.reserve(16);

    if (this->early_data_clear_.value() == EarlyDataClear::AUTO) {
      ESP_LOGV(TAG, "Auto-enabled early data clear for memory conservation");
    }
  }
}

// Periodically called to update forecast data.
void CWATownForecast::update() {
  if (!validate_config_()) {
    ESP_LOGE(TAG, "Configuration validation failed");
    return;
  }

  // If a retry is in progress, cancel it and start fresh
  if (this->retry_in_progress_) {
    this->cancel_timeout("cwa_retry");
    this->retry_in_progress_ = false;
    ESP_LOGD(TAG, "Cancelled pending retry, starting fresh request");
  }

  this->try_send_request_(0);
}

// Logs the component configuration.
void CWATownForecast::dump_config() {
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
  ESP_LOGCONFIG(TAG, "  Retry Count: %u", retry_count_.value());
  ESP_LOGCONFIG(TAG, "  Retry Delay: %u ms", retry_delay_.value());
  ESP_LOGCONFIG(TAG, "  PSRAM Available: %s", CWA_PSRAM_AVAILABLE() ? "true" : "false");
  LOG_UPDATE_INTERVAL(this);
}

// Validates the configuration parameters.
bool CWATownForecast::validate_config_() {
  bool valid = true;
  if (api_key_.value().empty()) {
    ESP_LOGE(TAG, "API Key not set");
    valid = false;
  }
  if (city_name_.value().empty()) {
    ESP_LOGE(TAG, "City name not set");
    valid = false;
  }
  {
    bool city_found = false;
    for (size_t i = 0; i < CITY_NAME_TO_3D_RESOURCE_ID_MAP_SIZE; ++i) {
      if (city_name_.value() == CITY_NAME_TO_3D_RESOURCE_ID_MAP[i].city) {
        city_found = true;
        break;
      }
    }
    if (!city_found) {
      ESP_LOGE(TAG, "Invalid city name: %s", city_name_.value().c_str());
      valid = false;
    }
  }
  if (town_name_.value().empty()) {
    ESP_LOGE(TAG, "Town name not set");
    valid = false;
  }
  if (!this->weather_elements_.empty()) {
    Mode mode = this->mode_;
    const char* const* valid_names;
    size_t valid_names_size;
    if (mode == Mode::THREE_DAYS) {
      valid_names = WEATHER_ELEMENT_NAMES_3DAYS;
      valid_names_size = WEATHER_ELEMENT_NAMES_3DAYS_SIZE;
    } else {
      valid_names = WEATHER_ELEMENT_NAMES_7DAYS;
      valid_names_size = WEATHER_ELEMENT_NAMES_7DAYS_SIZE;
    }
    for (const auto &element_name : this->weather_elements_) {
      bool found = false;
      for (size_t i = 0; i < valid_names_size; ++i) {
        if (element_name == valid_names[i]) {
          found = true;
          break;
        }
      }
      if (!found) {
        ESP_LOGE(TAG, "Invalid weather element for mode %s: %s", mode_to_string(mode).c_str(), element_name.c_str());
        valid = false;
      }
    }
  }

  return valid;
}

// Attempts to send HTTP request, scheduling retries via set_timeout() on failure.
void CWATownForecast::try_send_request_(uint32_t attempt) {
  uint32_t retry_count = this->retry_count_.value();

  if (attempt > 0) {
    ESP_LOGW(TAG, "Retrying request (attempt %u/%u)", attempt + 1, retry_count + 1);
  }
  ESP_LOGD(TAG, "HTTP request attempt %u/%u", attempt + 1, retry_count + 1);

  if (this->send_request_()) {
    // Success
    if (attempt > 0) {
      ESP_LOGI(TAG, "Request succeeded on attempt %u/%u", attempt + 1, retry_count + 1);
    }

    this->retry_in_progress_ = false;
    this->status_clear_warning();

    auto now = this->rtc_->now();
    if (this->last_success_) {
      this->last_success_->publish_state(now.strftime("%Y-%m-%d %H:%M:%S"));
    }

    this->publish_states_();

    std::tm tm = now.to_c_tm();
    time_t now_epoch = std::mktime(&tm);
    time_t expiry_offset = static_cast<time_t>(this->sensor_expiry_.value() / 1000);
    this->sensor_expiration_time_ = now_epoch + expiry_offset;

    if (!this->retain_fetched_data_.value()) {
      this->record_.release_data();
    }
    return;
  }

  // Failure — decide whether to retry or give up
  if (attempt < retry_count) {
    ESP_LOGW(TAG, "Request failed on attempt %u/%u, will retry", attempt + 1, retry_count + 1);

    uint32_t retry_delay = this->retry_delay_.value();
    uint32_t backoff_delay = retry_delay * (1 << attempt);
    uint32_t jitter = esp_random() % 1000;
    uint32_t total_delay = backoff_delay + jitter;
    if (total_delay > 30000) {
      total_delay = 30000;
    }

    ESP_LOGD(TAG, "Backoff delay: %u ms (base: %u ms, jitter: %u ms)", total_delay, backoff_delay, jitter);

    uint32_t next_attempt = attempt + 1;
    this->retry_in_progress_ = true;
    this->set_timeout("cwa_retry", total_delay, [this, next_attempt]() {
      this->try_send_request_(next_attempt);
    });
    return;
  }

  // Final failure
  ESP_LOGE(TAG, "Request failed after %u attempts", retry_count + 1);
  this->retry_in_progress_ = false;
  this->status_set_warning();
  this->on_error_trigger_.trigger();

  auto now = this->rtc_->now();
  if (this->last_error_) {
    this->last_error_->publish_state(now.strftime("%Y-%m-%d %H:%M:%S"));
  }

  std::tm tm = now.to_c_tm();
  time_t epoch = std::mktime(&tm);
  if (epoch > this->sensor_expiration_time_) {
    ESP_LOGW(TAG, "Sensor data expired");
    this->publish_states_();
  }

  if (!this->retain_fetched_data_.value()) {
    this->record_.release_data();
  }
}

// Sends HTTP request to fetch forecast data.
bool CWATownForecast::send_request_() {
  if (!this->http_request_) {
    ESP_LOGE(TAG, "HTTP request component not configured");
    return false;
  }

  if (!this->rtc_->now().is_valid()) {
    ESP_LOGW(TAG, "RTC is not valid");
    return false;
  }

  if (!network::is_connected()) {
    ESP_LOGW(TAG, "Network not connected");
    return false;
  }

  switch (early_data_clear_.value()) {
    case AUTO:
      if (!CWA_PSRAM_AVAILABLE()) {
        ESP_LOGD(TAG, "[Auto] Clear forecast data before sending request");
        this->record_.release_data();
      }
      break;

    case ON:
      ESP_LOGD(TAG, "[On] Clear forecast data before sending request");
      this->record_.release_data();
      break;
  }

  std::string city_name = city_name_.value();

  // Get the appropriate resource ID based on forecast mode
  Mode mode = mode_;
  const char* resource_id = find_city_resource_id(city_name, mode == Mode::SEVEN_DAYS);
  if (!resource_id || strlen(resource_id) == 0) {
    ESP_LOGE(TAG, "Invalid city name: %s", city_name.c_str());
    return false;
  }

  ESP_LOGD(TAG, "City name: %s, Town name: %s, Mode: %s", city_name.c_str(), town_name_.value().c_str(), mode_to_string(mode).c_str());
  ESP_LOGD(TAG, "Resource ID: %s", resource_id);
  std::string encoded_town_name = url_encode(town_name_.value());
  std::string element_param;
  if (!this->weather_elements_.empty()) {
    std::string joined;
    bool first = true;
    for (const auto &name : this->weather_elements_) {
      if (!first) joined += ",";
      joined += url_encode(name);
      first = false;
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
      std::string encoded_buffer = url_encode(std::string(buffer));
      time_to_param = "&timeTo=" + encoded_buffer;
    } else {
      ESP_LOGW(TAG, "Ignoring timeTo parameter, RTC not set");
    }
  }
  std::string url = "https://opendata.cwa.gov.tw/api/v1/rest/datastore/" + std::string(resource_id) + "?Authorization=" + api_key_.value() +
                    "&format=JSON&LocationName=" + encoded_town_name + element_param + time_to_param;

  ESP_LOGD(TAG, "Sending query: %s", url.c_str());

  // Build HTTP request using ESPHome's http_request component
  App.feed_wdt();
  auto container = this->http_request_->get(url);

  bool request_success = false;
  uint64_t hash_code = 0;

  if (container == nullptr) {
    ESP_LOGE(TAG, "HTTP request failed: no response container");
  } else if (container->status_code != 200) {
    ESP_LOGE(TAG, "HTTP request failed with code: %d", container->status_code);
  } else {
    App.feed_wdt();
    ESP_LOGD(TAG, "HTTP 200 OK, content_length: %zu", container->content_length);

    // Wrap container with our stream adapter for streaming JSON parsing
    HttpStreamAdapter stream(container, 1024, this->http_request_->get_timeout()); // 1KB buffer

    // Add timeout protection for response processing
    unsigned long process_start = millis();
    uint32_t max_process_time = this->http_request_->get_timeout() + 10000; // Add 10s buffer for processing

    request_success = this->process_response_(stream, hash_code);

    unsigned long process_duration = millis() - process_start;
    if (process_duration > max_process_time) {
      ESP_LOGW(TAG, "Response processing took too long: %lu ms (max: %u ms)", process_duration, max_process_time);
      request_success = false;
    }

    ESP_LOGD(TAG, "Total bytes read from stream: %zu", stream.getBytesRead());
    ESP_LOGD(TAG, "Response processing duration: %lu ms", process_duration);

    // Drain any remaining buffered data
    stream.drainBuffer();
  }

  if (container) {
    container->end();
  }
  container.reset();

  if (request_success) {
    if (this->check_changes(hash_code)) {
      ESP_LOGD(TAG, "Triggering on_data_change");
      this->on_data_change_trigger_.trigger(this->record_);
    } else {
      ESP_LOGD(TAG, "No data change detected");
    }
    return true;
  } else {
    ESP_LOGE(TAG, "Failed to parse JSON response");
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

bool CWATownForecast::parse_to_record(HttpStreamAdapter &stream, Record& record, uint64_t &hash_code) {
  if (!stream.find("\"success\":")) {
    ESP_LOGE(TAG, "Could not find success field");
    return false;
  }
  std::string success_val = stream.readStringUntil(',');
  if (success_val != "\"true\"") {
    ESP_LOGE(TAG, "API response 'success' is not true: %s", success_val.c_str());
    return false;
  }
  
  record.mode = this->mode_;
  record.weather_elements.reserve(this->mode_ == Mode::THREE_DAYS ? WEATHER_ELEMENT_NAMES_3DAYS_SIZE : WEATHER_ELEMENT_NAMES_7DAYS_SIZE);
  
  if (!stream.find("\"LocationsName\":\"")) {
    ESP_LOGE(TAG, "Could not find LocationsName");
    return false;
  }
  record.locations_name = stream.readStringUntil('"');

  if (!stream.find("\"LocationName\":\"")) {
    ESP_LOGE(TAG, "Could not find LocationName");
    return false;
  }
  record.location_name = stream.readStringUntil('"');

  auto parse_coordinate = [&](const char *key, double &out) -> bool {
    if (!stream.find(key)) {
      ESP_LOGE(TAG, "Could not find %s", key);
      return false;
    }
    std::string str = stream.readStringUntil('"');
    const char *cstr = str.c_str();
    char *end = nullptr;
    double val = std::strtod(cstr, &end);
    if (cstr == end || *end != '\0') {
      ESP_LOGW(TAG, "Invalid coordinate value for %s: %s", key, cstr);
      out = NAN;
    } else {
      out = val;
    }
    return true;
  };

  if (!parse_coordinate("\"Latitude\":\"", record.latitude)) return false;
  if (!parse_coordinate("\"Longitude\":\"", record.longitude)) return false;

  if (!stream.find("\"WeatherElement\":[")) {
    ESP_LOGE(TAG, "Could not find WeatherElement array");
    return false;
  }

  ESPTime now = this->rtc_->now();
  SunSet sun;
  double timezone_offset = static_cast<double>(now.timezone_offset()) / 3600;
  sun.setPosition(record.latitude, record.longitude, timezone_offset);
  ESP_LOGD(TAG, "Sunset Latitude: %f, Longitude: %f, Offset: %.0f", record.latitude, record.longitude, timezone_offset);

#ifdef USE_PSRAM
  // PSRAM-aware allocator for ArduinoJson to offload JSON parsing buffers from internal RAM
  struct SpiRamJsonAllocator : ArduinoJson::Allocator {
    void *allocate(size_t size) override {
      return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    void deallocate(void *ptr) override { heap_caps_free(ptr); }
    void *reallocate(void *ptr, size_t new_size) override {
      return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
  };
  SpiRamJsonAllocator psram_json_allocator;
  ArduinoJson::JsonDocument time_obj(CWA_PSRAM_AVAILABLE() ? &psram_json_allocator : nullptr);
#else
  ArduinoJson::JsonDocument time_obj;
#endif
  bool has_valid_data = false;  // Track if we have at least one element with valid data
  do {
    App.feed_wdt();
    WeatherElement we;
    if (!stream.find("\"ElementName\":\"")) {
      ESP_LOGE(TAG, "Could not find ElementName");
      return false;
    }
    we.element_name = stream.readStringUntil('"');
    if (!stream.find("\"Time\":[")) {
      ESP_LOGE(TAG, "Could not find Time array for %s", we.element_name.c_str());
      return false;
    }

    ESP_LOGV(TAG, "Processing Weather Element: %s", we.element_name.c_str());

    // check for empty array
    int next = stream.peek();
    if (next == ']') {
      ESP_LOGW(TAG, "Empty Time array for %s, skipping element processing", we.element_name.c_str());
      stream.read();
      continue;
    }

    // Mark that we found at least one element with data
    has_valid_data = true;

    do {
      time_obj.clear();      
      ESP_LOGV(TAG, "Parsing JSON with %d bytes available", stream.available());
      
      // The ReadLoggingStream will output the stream content to Serial for debugging
      DeserializationError err = deserializeJson(time_obj, stream);
      if (err) {
        ESP_LOGE(TAG, "JSON parsing failed: %s", err.c_str());
        ESP_LOGE(TAG, "Stream available bytes before error: %d", stream.available());
        
        if (err == DeserializationError::IncompleteInput) {
          ESP_LOGE(TAG, "IncompleteInput detected - JSON document was truncated");
          ESP_LOGE(TAG, "Current memory state - free heap: %u, max block: %u", 
                   esp_get_free_heap_size(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
          
          // For IncompleteInput, we should abort the entire parsing process
          // rather than try to recover, as the data integrity is compromised
          ESP_LOGE(TAG, "Aborting parse due to incomplete JSON data");
          return false;
        } else {
          ESP_LOGE(TAG, "JSON parsing failed with error: %s", err.c_str());
          return false;
        }
      }

      Time ts;
      ts.element_values.reserve(3);
      
      if (time_obj["DataTime"].is<std::string>()) {
        std::string tmp = time_obj["DataTime"].as<std::string>();
        std::tm temp_tm;
        if (!parse_iso8601(tmp, temp_tm)) {
          ESP_LOGE(TAG, "Could not parse DataTime: %s", tmp.c_str());
          return false;
        }
        ts.data_time = temp_tm;
      }
      if (time_obj["StartTime"].is<std::string>()) {
        std::string tmp = time_obj["StartTime"].as<std::string>();
        std::tm temp_tm;
        if (!parse_iso8601(tmp, temp_tm)) {
          ESP_LOGE(TAG, "Could not parse StartTime: %s", tmp.c_str());
          return false;
        }
        ts.start_time_data = temp_tm;
      }
      if (time_obj["EndTime"].is<std::string>()) {
        std::string tmp = time_obj["EndTime"].as<std::string>();
        std::tm temp_tm;
        if (!parse_iso8601(tmp, temp_tm)) {
          ESP_LOGE(TAG, "Could not parse EndTime: %s", tmp.c_str());
          return false;
        }
        ts.end_time_data = temp_tm;
      }

      // Process element values
      for (ArduinoJson::JsonObject val_obj : time_obj["ElementValue"].as<ArduinoJson::JsonArray>()) {
        for (ArduinoJson::JsonPair kv : val_obj) {
          ElementValueKey evk;
          if (parse_element_value_key(kv.key().c_str(), evk)) {
            auto value = kv.value().as<std::string>();
            auto it = std::find_if(ts.element_values.begin(), ts.element_values.end(),
                                   [&](const std::pair<ElementValueKey, PsramString> &p) { return p.first == evk; });
            if (it != ts.element_values.end())
              it->second = PsramString(value);
            else
              ts.element_values.emplace_back(evk, PsramString(value));

            // Special handling for weather codes to generate weather icons
            if (we.element_name == WEATHER_ELEMENT_NAME_WEATHER && evk == ElementValueKey::WEATHER_CODE) {
              const char* icon_name = find_weather_icon(value);
              if (icon_name && strlen(icon_name) > 0) {
                std::string icon = icon_name;

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

                if (t.tm_hour < sunrise_hour || t.tm_hour >= sunset_hour) {
                  if (icon == "sunny") {
                    icon = "night";
                  } else if (icon == "partly-cloudy" || icon == "cloudy") {
                    icon = "night-partly-cloudy";
                  }
                }
                ts.element_values.emplace_back(ElementValueKey::WEATHER_ICON, PsramString(icon));
              } else {
                ts.element_values.emplace_back(ElementValueKey::WEATHER_ICON, PsramString(""));
              }
            }
          } else {
            ESP_LOGW(TAG, "Unknown element value key: %s", kv.key().c_str());
          }
        }
      }
      we.times.push_back(std::move(ts));
    } while (stream.findUntil(",", "]"));
    record.weather_elements.push_back(std::move(we));
  } while (stream.findUntil(",", "]"));

  // Check if we got any valid data
  if (!has_valid_data) {
    ESP_LOGE(TAG, "API response has no valid data - all Time arrays are empty");
    return false;
  }

  // Determine start and end time for the entire record
  bool first_time = true;
  std::tm min_tm{};
  std::tm max_tm{};
  for (const auto &we : record.weather_elements) {
    for (const auto &t : we.times) {
      std::tm candidate_tm{};
      if (t.data_time.is_valid()) {
        candidate_tm = t.data_time.to_tm();
      } else if (t.start_time_data.is_valid()) {
        candidate_tm = t.start_time_data.to_tm();
      } else {
        continue;
      }
      std::time_t cand_epoch = std::mktime(&candidate_tm);
      if (first_time || cand_epoch < std::mktime(&min_tm)) {
        min_tm = candidate_tm;
      }
      if (first_time) {
        if (t.end_time_data.is_valid()) {
          max_tm = t.end_time_data.to_tm();
        } else {
          max_tm = candidate_tm;
        }
      } else if (t.end_time_data.is_valid()) {
        std::tm end_tm = t.end_time_data.to_tm();
        std::time_t end_epoch = std::mktime(&end_tm);
        if (end_epoch > std::mktime(&max_tm)) {
          max_tm = end_tm;
        }
      }
      first_time = false;
    }
  }
  if (!first_time) {
    record.start_time = min_tm;
    record.end_time = max_tm;
  }

  // Set the updated time to current time
  if (now.is_valid()) {
    record.updated_time = now.to_c_tm();
  }

  // Calculate hash code for change detection
  uint64_t new_hash = 0;
  const uint64_t salt = 0x9e3779b97f4a7c15ULL;
  auto combine_hash = [&](uint64_t h) {
    new_hash ^= h + salt + (new_hash << 6) + (new_hash >> 2);
  };
  auto combine_int = [&](uint64_t v) {
    // Murmur-style integer mix
    v ^= v >> 33;
    v *= 0xff51afd7ed558ccdULL;
    v ^= v >> 33;
    combine_hash(v);
  };
  // Hash raw char data directly — avoids temporary std::string allocation
  auto combine_chars = [&](const char* data, size_t len) {
    uint64_t h = 0xcbf29ce484222325ULL;  // FNV-1a offset basis
    for (size_t i = 0; i < len; ++i) {
      h ^= static_cast<uint64_t>(static_cast<unsigned char>(data[i]));
      h *= 0x100000001b3ULL;  // FNV-1a prime
    }
    combine_hash(h);
  };
  combine_chars(record.locations_name.c_str(), record.locations_name.size());
  combine_chars(record.location_name.c_str(), record.location_name.size());
  for (const auto &we : record.weather_elements) {
    combine_chars(we.element_name.c_str(), we.element_name.size());
    for (const auto &ts : we.times) {
      if (ts.data_time.is_valid()) {
        std::tm tm_copy = ts.data_time.to_tm();
        combine_int(static_cast<uint64_t>(std::mktime(&tm_copy)));
      } else if (ts.start_time_data.is_valid() && ts.end_time_data.is_valid()) {
        std::tm tm1 = ts.start_time_data.to_tm();
        std::tm tm2 = ts.end_time_data.to_tm();
        combine_int(static_cast<uint64_t>(std::mktime(&tm1)));
        combine_int(static_cast<uint64_t>(std::mktime(&tm2)));
      }

      for (const auto &p : ts.element_values) {
        combine_int(static_cast<uint64_t>(p.first));
        combine_chars(p.second.c_str(), p.second.size());
      }
    }
  }
  
  hash_code = new_hash;
  return true;
}

// Processes the HTTP response and parses forecast data.
// Two strategies: PSRAM devices parse into a PSRAM-allocated temp record for
// atomicity; non-PSRAM devices clear existing data, parse in place, and
// discard partial results on failure.
bool CWATownForecast::process_response_(HttpStreamAdapter &stream, uint64_t &hash_code) {
  if (CWA_PSRAM_AVAILABLE()) {
    ESP_LOGD(TAG, "Using PSRAM buffer strategy");
    Record *temp = static_cast<Record *>(heap_caps_malloc(sizeof(Record), MALLOC_CAP_SPIRAM));
    if (temp) {
      new (temp) Record();
      bool ok = parse_to_record(stream, *temp, hash_code);
      if (ok) {
        this->record_ = std::move(*temp);
      }
      temp->~Record();
      heap_caps_free(temp);
      return ok;
    }
    ESP_LOGW(TAG, "PSRAM allocation failed, falling back to in-place strategy");
  }

  ESP_LOGD(TAG, "Using in-place parse strategy");
  this->record_.release_data();

  if (!parse_to_record(stream, this->record_, hash_code)) {
    ESP_LOGW(TAG, "Parse failed, clearing record");
    this->record_.release_data();
    return false;
  }
  return true;
}

// Returns the latest forecast data record.
Record &CWATownForecast::get_data() {
  if (!this->retain_fetched_data_.value()) {
    ESP_LOGE(TAG, "Turn on retain_fetched_data option to get forecast data");
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
  const char* element_name = find_mode_element_name(this->record_.mode, key);
  if (!element_name) {
    ESP_LOGE(TAG, "Invalid element value key for mode %s: %s",
             mode_to_string(this->record_.mode).c_str(), element_value_key_to_string(key).c_str());
    publish_no_match(sensor);
    return;
  }

  const WeatherElement *we = this->record_.find_weather_element(element_name);
  if (we && !we->times.empty()) {
    Time *ts = we->match_time(target_tm, key, fallback_to_first);
    if (ts) {
#if ESP_LOG_LEVEL >= ESP_LOG_VERBOSE
      if (ts->data_time.is_valid())
        ESP_LOGV(TAG, "matched (%s): %s", element_name, tm_to_esptime(ts->data_time.to_tm()).strftime("%Y-%m-%d %H:%M").c_str());
      else if (ts->start_time_data.is_valid() && ts->end_time_data.is_valid())
        ESP_LOGV(TAG, "matched (%s): %s - %s", element_name, tm_to_esptime(ts->start_time_data.to_tm()).strftime("%Y-%m-%d %H:%M").c_str(),
                 tm_to_esptime(ts->end_time_data.to_tm()).strftime("%Y-%m-%d %H:%M").c_str());
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
    ESP_LOGW(TAG, "No match found for %s", element_name);
    publish_no_match(sensor);
  } else {
    ESP_LOGW(TAG, "No weather element found for %s", element_name);
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
