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

#include "esphome/components/network/util.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/watchdog/watchdog.h"
#include "esphome/core/helpers.h"
#include "esphome/core/time.h"

namespace esphome {
namespace cwa_town_forecast {

// Returns the setup priority for the component.
float CWATownForecast::get_setup_priority() const { return setup_priority::LATE; }

// Initializes the component.
void CWATownForecast::setup() {
  if (psramFound()) {
    ESP_LOGI(TAG, "PSRAM detected (%d bytes), using extended memory allocation", ESP.getPsramSize());
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

  auto now = this->rtc_->now();
  std::tm tm = now.to_c_tm();
  if (send_request_with_retry_()) {
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
  ESP_LOGCONFIG(TAG, "  Retry Count: %u", retry_count_.value());
  ESP_LOGCONFIG(TAG, "  Retry Delay: %u ms", retry_delay_.value());
  ESP_LOGCONFIG(TAG, "  PSRAM Available: %s", psram_available ? "true" : "false");
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
  if (CITY_NAMES.find(city_name_.value()) == CITY_NAMES.end()) {
    ESP_LOGE(TAG, "Invalid city name: %s", city_name_.value().c_str());
    valid = false;
  }
  if (town_name_.value().empty()) {
    ESP_LOGE(TAG, "Town name not set");
    valid = false;
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
        valid = false;
      }
    }
  }

  if (sensor_expiry_.value() < 0) {
    ESP_LOGE(TAG, "Sensor Expiry must be greater or equal to 0");
    valid = false;
  }
  return valid;
}

// Sends HTTP request with retry mechanism.
bool CWATownForecast::send_request_with_retry_() {
  uint32_t retry_count = retry_count_.value();
  uint32_t retry_delay = retry_delay_.value();
  
  // If retry count is 0, just make one attempt
  if (retry_count == 0) {
    return send_request_();
  }
  
  for (uint32_t attempt = 0; attempt <= retry_count; ++attempt) {
    if (attempt > 0) {
      ESP_LOGW(TAG, "Retrying request (attempt %u/%u)", attempt + 1, retry_count + 1);
      
      // Calculate exponential backoff with jitter
      uint32_t backoff_delay = retry_delay * (1 << (attempt - 1)); // Exponential backoff
      uint32_t jitter = random() % 1000; // Add up to 1 second of jitter
      uint32_t total_delay = backoff_delay + jitter;
      
      // Cap the delay to prevent excessive waiting
      if (total_delay > 30000) { // Max 30 seconds
        total_delay = 30000;
      }
      
      ESP_LOGD(TAG, "Backoff delay: %u ms (base: %u ms, jitter: %u ms)", total_delay, backoff_delay, jitter);
      
      // Use non-blocking delay to avoid blocking the main loop
      uint32_t delay_end = millis() + total_delay;
      while (millis() < delay_end) {
        App.feed_wdt();
        delay(100); // Small delay to prevent busy waiting
      }
    }
    
    ESP_LOGD(TAG, "HTTP request attempt %u/%u", attempt + 1, retry_count + 1);
    
    if (send_request_()) {
      if (attempt > 0) {
        ESP_LOGI(TAG, "Request succeeded on attempt %u/%u", attempt + 1, retry_count + 1);
      }
      return true;
    }
    
    if (attempt < retry_count) {
      ESP_LOGW(TAG, "Request failed on attempt %u/%u, will retry", attempt + 1, retry_count + 1);
    }
  }
  
  ESP_LOGE(TAG, "Request failed after %u attempts", retry_count + 1);
  return false;
}

// Sends HTTP request to fetch forecast data.
bool CWATownForecast::send_request_() {
  if (!this->rtc_->now().is_valid()) {
    ESP_LOGW(TAG, "RTC is not valid");
    return false;
  }

  if (!network::is_connected()) {
    ESP_LOGW(TAG, "Network not connected");
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
  const char* resource_id = find_city_resource_id(city_name, mode == Mode::SEVEN_DAYS);
  if (!resource_id || strlen(resource_id) == 0) {
    ESP_LOGE(TAG, "Invalid city name: %s", city_name.c_str());
    return false;
  }

  ESP_LOGD(TAG, "City name: %s, Town name: %s, Mode: %s", city_name.c_str(), town_name_.value().c_str(), mode_to_string(mode).c_str());
  ESP_LOGD(TAG, "Resource ID: %s", resource_id);
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
  std::string url = "https://opendata.cwa.gov.tw/api/v1/rest/datastore/" + std::string(resource_id) + "?Authorization=" + api_key_.value() +
                    "&format=JSON&LocationName=" + std::string(encoded_town_name.c_str()) + element_param + time_to_param;

  HTTPClient http;
  http.useHTTP10(true);
  http.setConnectTimeout(http_connect_timeout_.value());
  http.setTimeout(http_timeout_.value());
  http.addHeader("Content-Type", "application/json");
  http.begin(url.c_str());
  ESP_LOGD(TAG, "Sending query: %s", url.c_str());
  ESP_LOGD(TAG, "Before request: free heap:%u, max block:%u", ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
  
  // Start timer for overall request timeout protection
  unsigned long request_start = millis();
  uint32_t max_request_time = http_connect_timeout_.value() + http_timeout_.value() + 5000; // Add 5s buffer
  
  App.feed_wdt();
  int http_code = http.GET();
  
  // Check if request took too long
  unsigned long request_duration = millis() - request_start;
  if (request_duration > max_request_time) {
    ESP_LOGW(TAG, "HTTP request took too long: %lu ms (max: %u ms)", request_duration, max_request_time);
    http.end();
    return false;
  }
  ESP_LOGD(TAG, "After request: free heap:%u, max block:%u", ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
  
  // Use RAII pattern to ensure HTTP connection is always closed
  bool request_success = false;
  uint64_t hash_code = 0;
  
  if (http_code == HTTP_CODE_OK) {
    App.feed_wdt();
    
    // Get stream and set timeout for read operations
    auto& stream = http.getStream();
    
    // Wrap stream with our buffered wrapper for IncompleteInput debugging
    BufferedStream bufferedStream(stream, 1024); // 1KB buffer
    ESP_LOGD(TAG, "Using BufferedStream (1KB buffer) to improve JSON parsing reliability and monitor stream reading");
    
    // Add timeout protection for response processing
    unsigned long process_start = millis();
    uint32_t max_process_time = http_timeout_.value() + 10000; // Add 10s buffer for processing
    
    request_success = this->process_response_(bufferedStream, hash_code);
    
    unsigned long process_duration = millis() - process_start;
    if (process_duration > max_process_time) {
      ESP_LOGW(TAG, "Response processing took too long: %lu ms (max: %u ms)", process_duration, max_process_time);
      request_success = false;
    }
    
    ESP_LOGD(TAG, "Total bytes read from stream: %d", bufferedStream.getBytesRead());
    ESP_LOGD(TAG, "Response processing duration: %lu ms", process_duration);
    
    // Drain any remaining buffered data to avoid SSL state issues
    bufferedStream.drainBuffer();
  } else {
    ESP_LOGE(TAG, "HTTP request failed with code: %d", http_code);
  }

  // Always end HTTP connection, regardless of success or failure
  http.end();
  ESP_LOGD(TAG, "After json parse: free heap:%u, max block:%u", ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));

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

// Memory management helper methods
MinimalCheckpoint CWATownForecast::create_minimal_checkpoint(const Record& record) {
  MinimalCheckpoint checkpoint;
  checkpoint.locations_name = record.locations_name;
  checkpoint.location_name = record.location_name;
  checkpoint.latitude = record.latitude;
  checkpoint.longitude = record.longitude;
  checkpoint.mode = record.mode;
  checkpoint.element_count = record.weather_elements.size();
  checkpoint.valid = true;
  return checkpoint;
}

void CWATownForecast::restore_from_checkpoint(Record& record, const MinimalCheckpoint& checkpoint) {
  if (checkpoint.valid) {
    record.locations_name = checkpoint.locations_name;
    record.location_name = checkpoint.location_name;
    record.latitude = checkpoint.latitude;
    record.longitude = checkpoint.longitude;
    record.mode = checkpoint.mode;
    record.weather_elements.clear();
    if (checkpoint.element_count > 0) {
      record.weather_elements.reserve(checkpoint.element_count);
    }
  }
}

bool CWATownForecast::parse_to_record(Stream &stream, Record& record, uint64_t &hash_code) {
  if (!stream.find("\"success\":")) {
    ESP_LOGE(TAG, "Could not find success field");
    return false;
  }
  String success_val = stream.readStringUntil(',');
  if (success_val != "\"true\"") {
    ESP_LOGE(TAG, "API response 'success' is not true: %s", success_val.c_str());
    return false;
  }
  
  record.mode = this->mode_;
  record.weather_elements.reserve(this->mode_ == Mode::THREE_DAYS ? WEATHER_ELEMENT_NAMES_3DAYS.size() : WEATHER_ELEMENT_NAMES_7DAYS.size());
  
  if (!stream.find("\"LocationsName\":\"")) {
    ESP_LOGE(TAG, "Could not find LocationsName");
    return false;
  }
  record.locations_name = stream.readStringUntil('"').c_str();

  if (!stream.find("\"LocationName\":\"")) {
    ESP_LOGE(TAG, "Could not find LocationName");
    return false;
  }
  record.location_name = stream.readStringUntil('"').c_str();

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
    record.latitude = NAN;
  } else {
    record.latitude = lat_val;
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
    record.longitude = NAN;
  } else {
    record.longitude = lon_val;
  }

  if (!stream.find("\"WeatherElement\":[")) {
    ESP_LOGE(TAG, "Could not find WeatherElement array");
    return false;
  }

  ESPTime now = this->rtc_->now();
  SunSet sun;
  double timezone_offset = static_cast<double>(now.timezone_offset()) / 3600;
  sun.setPosition(record.latitude, record.longitude, timezone_offset);
  ESP_LOGD(TAG, "Sunset Latitude: %f, Longitude: %f, Offset: %d", record.latitude, record.longitude, timezone_offset);

  ArduinoJson::JsonDocument time_obj;
  bool has_valid_data = false;  // Track if we have at least one element with valid data
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
                   ESP.getFreeHeap(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
          
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
                                   [&](const std::pair<ElementValueKey, AdaptiveString> &p) { return p.first == evk; });
            if (it != ts.element_values.end())
              it->second = AdaptiveString(value);
            else
              ts.element_values.emplace_back(evk, AdaptiveString(value));

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
                ts.element_values.emplace_back(ElementValueKey::WEATHER_ICON, AdaptiveString(icon));
              } else {
                ts.element_values.emplace_back(ElementValueKey::WEATHER_ICON, AdaptiveString(""));
              }
            }
          } else {
            ESP_LOGW(TAG, "Unknown element value key: %s", kv.key().c_str());
          }
        }
        we.times.push_back(std::move(ts));
      }
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
  std::hash<std::string> hasher;
  const uint64_t salt = 0x9e3779b97f4a7c15ULL;
  auto combine = [&](const std::string &s) {
    uint64_t h = hasher(s);
    new_hash ^= h + salt + (new_hash << 6) + (new_hash >> 2);
  };
  combine(record.locations_name);
  combine(record.location_name);
  for (const auto &we : record.weather_elements) {
    combine(we.element_name);
    for (const auto &ts : we.times) {
      if (ts.data_time.is_valid()) {
        std::tm tm_copy = ts.data_time.to_tm();
        time_t t = std::mktime(&tm_copy);
        combine(std::to_string(t));
      } else if (ts.start_time_data.is_valid() && ts.end_time_data.is_valid()) {
        std::tm tm1 = ts.start_time_data.to_tm();
        std::tm tm2 = ts.end_time_data.to_tm();
        time_t t1 = std::mktime(&tm1);
        time_t t2 = std::mktime(&tm2);
        combine(std::to_string(t1));
        combine(std::to_string(t2));
      }

      for (const auto &p : ts.element_values) {
        combine(element_value_key_to_string(p.first) + p.second.to_std_string());
      }
    }
  }
  
  hash_code = new_hash;
  return true;
}

// Processes the HTTP response and parses forecast data.
bool CWATownForecast::process_response_(Stream &stream, uint64_t &hash_code) {
  // Use adaptive memory management strategy
  return process_response_with_adaptive_strategy(stream, hash_code);
}

// AdaptiveMemoryManager Implementation
MemoryStats AdaptiveMemoryManager::get_current_stats() {
  MemoryStats stats;
  stats.free_heap = ESP.getFreeHeap();
  stats.max_block = ESP.getMaxAllocHeap();
  stats.total_heap = ESP.getHeapSize();
  stats.has_psram = psramFound();
  
  if (stats.has_psram) {
    stats.psram_free = ESP.getFreePsram();
    stats.psram_total = ESP.getPsramSize();
  }
  
  // Calculate fragmentation ratio
  if (stats.free_heap > 0) {
    stats.fragmentation_ratio = 1.0f - (static_cast<float>(stats.max_block) / static_cast<float>(stats.free_heap));
  }
  
  return stats;
}

MemoryStrategy AdaptiveMemoryManager::select_optimal_strategy(const MemoryStats& stats) {
  // Strategy 1: PSRAM available - use dual buffer
  if (stats.has_psram && stats.psram_free > 50000) {
    return MemoryStrategy::PSRAM_DUAL_BUFFER;
  }
  
  // Strategy 2: Sufficient internal memory - use checkpoint
  if (is_memory_sufficient_for_temp_record(stats)) {
    return MemoryStrategy::INTERNAL_CHECKPOINT;
  }
  
  // Strategy 3: High fragmentation - use adaptive approach
  if (stats.fragmentation_ratio > MAX_FRAGMENTATION_RATIO) {
    return MemoryStrategy::ADAPTIVE_FRAGMENT;
  }
  
  // Strategy 4: Low memory fallback - minimal streaming
  return MemoryStrategy::STREAM_MINIMAL;
}

void AdaptiveMemoryManager::log_memory_decision(MemoryStrategy strategy, const MemoryStats& stats) {
  const char* strategy_names[] = {"PSRAM_DUAL_BUFFER", "INTERNAL_CHECKPOINT", "ADAPTIVE_FRAGMENT", "STREAM_MINIMAL"};
  ESP_LOGD(TAG, "Memory strategy: %s", strategy_names[static_cast<int>(strategy)]);
  ESP_LOGD(TAG, "  Free heap: %d bytes, Max block: %d bytes, Fragmentation: %.2f%%", 
           stats.free_heap, stats.max_block, stats.fragmentation_ratio * 100.0f);
  if (stats.has_psram) {
    ESP_LOGD(TAG, "  PSRAM available: %d bytes free / %d bytes total", stats.psram_free, stats.psram_total);
  }
}

bool AdaptiveMemoryManager::should_use_psram_allocation(const MemoryStats& stats) {
  return stats.has_psram && stats.psram_free > 50000;
}

bool AdaptiveMemoryManager::is_memory_sufficient_for_temp_record(const MemoryStats& stats) {
  return stats.free_heap > MIN_HEAP_FOR_TEMP_RECORD && stats.max_block > MIN_BLOCK_FOR_TEMP_RECORD;
}

bool CWATownForecast::process_response_with_adaptive_strategy(Stream &stream, uint64_t &hash_code) {
  auto stats = AdaptiveMemoryManager::get_current_stats();
  auto strategy = AdaptiveMemoryManager::select_optimal_strategy(stats);
  AdaptiveMemoryManager::log_memory_decision(strategy, stats);
  
  switch (strategy) {
    case MemoryStrategy::PSRAM_DUAL_BUFFER:
      return process_with_psram_dual_buffer(stream, this->record_, hash_code);
    case MemoryStrategy::INTERNAL_CHECKPOINT:
      return process_with_internal_checkpoint(stream, this->record_, hash_code);
    case MemoryStrategy::ADAPTIVE_FRAGMENT:
      return process_with_adaptive_fragment(stream, this->record_, hash_code);
    case MemoryStrategy::STREAM_MINIMAL:
      return process_with_stream_minimal(stream, this->record_, hash_code);
    default:
      ESP_LOGE(TAG, "Unknown memory strategy");
      return false;
  }
}

bool CWATownForecast::process_with_psram_dual_buffer(Stream &stream, Record& record, uint64_t &hash_code) {
  ESP_LOGD(TAG, "Using PSRAM dual buffer strategy");
  
  Record* temp_record = (Record*)heap_caps_malloc(sizeof(Record), MALLOC_CAP_SPIRAM);
  if (!temp_record) {
    ESP_LOGE(TAG, "Failed to allocate temp_record in PSRAM, falling back to internal RAM");
    return process_with_internal_checkpoint(stream, record, hash_code);
  }
  
  // Use placement new to construct Record in PSRAM
  new(temp_record) Record();
  
  bool parse_success = parse_to_record(stream, *temp_record, hash_code);
  if (parse_success) {
    record = std::move(*temp_record);
  }
  
  // Clean up regardless of success/failure
  temp_record->~Record();
  heap_caps_free(temp_record);
  
  if (!parse_success) {
    return false;
  }
  
  ESP_LOGD(TAG, "PSRAM dual buffer strategy completed successfully");
  return true;
}

bool CWATownForecast::process_with_internal_checkpoint(Stream &stream, Record& record, uint64_t &hash_code) {
  ESP_LOGD(TAG, "Using internal RAM checkpoint strategy");
  
  auto checkpoint = create_minimal_checkpoint(record);
  record.weather_elements.clear();
  
  if (!parse_to_record(stream, record, hash_code)) {
    ESP_LOGW(TAG, "Parse failed, restoring from checkpoint");
    restore_from_checkpoint(record, checkpoint);
    return false;
  }
  
  ESP_LOGD(TAG, "Internal RAM checkpoint strategy completed successfully");
  return true;
}

bool CWATownForecast::process_with_adaptive_fragment(Stream &stream, Record& record, uint64_t &hash_code) {
  ESP_LOGD(TAG, "Using adaptive fragmentation strategy");
  
  // Try PSRAM first if available
  auto stats = AdaptiveMemoryManager::get_current_stats();
  if (stats.has_psram && stats.psram_free > 30000) {
    ESP_LOGD(TAG, "Adaptive strategy: Using PSRAM");
    return process_with_psram_dual_buffer(stream, record, hash_code);
  }
  
  // Check if we have enough contiguous memory for temp record
  if (stats.max_block > 25000) {
    ESP_LOGD(TAG, "Adaptive strategy: Using temp record approach");
    Record temp_record;
    if (!parse_to_record(stream, temp_record, hash_code)) {
      return false;
    }
    record = std::move(temp_record);
    ESP_LOGD(TAG, "Adaptive fragmentation strategy completed successfully");
    return true;
  }
  
  // Fall back to checkpoint strategy
  ESP_LOGD(TAG, "Adaptive strategy: Falling back to checkpoint");
  return process_with_internal_checkpoint(stream, record, hash_code);
}

bool CWATownForecast::process_with_stream_minimal(Stream &stream, Record& record, uint64_t &hash_code) {
  ESP_LOGD(TAG, "Using stream minimal strategy");
  
  // Clear old data to free memory
  record.weather_elements.clear();
  
  // Use minimal memory parsing directly into record
  if (!parse_to_record(stream, record, hash_code)) {
    ESP_LOGW(TAG, "Stream minimal parse failed");
    return false;
  }
  
  ESP_LOGD(TAG, "Stream minimal strategy completed successfully");
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
      if (ts->data_time.is_valid())
        ESP_LOGV(TAG, "matched (%s): %s", element_name.c_str(), tm_to_esptime(ts->data_time.to_tm()).strftime("%Y-%m-%d %H:%M").c_str());
      else if (ts->start_time_data.is_valid() && ts->end_time_data.is_valid())
        ESP_LOGV(TAG, "matched (%s): %s - %s", element_name.c_str(), tm_to_esptime(ts->start_time_data.to_tm()).strftime("%Y-%m-%d %H:%M").c_str(),
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
