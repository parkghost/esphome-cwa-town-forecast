# ESPHome CWA Town Forecast Component

This is an external component for ESPHome that fetches town weather forecast data from Taiwan's Central Weather Administration (CWA) Open Data platform.

> For a complete example using this component with an e-paper display, see the [Weather Panel YAML](https://github.com/parkghost/esphome-epaper-examples/tree/main?tab=readme-ov-file#weather-panelyaml) in the [esphome-epaper-examples](https://github.com/parkghost/esphome-epaper-examples) repository.

![weather-panel.yaml](https://raw.githubusercontent.com/parkghost/esphome-epaper-examples/main/images/weather-panel.jpg)

## Prerequisites

- You need a CWA Open Data API authorization key. Obtain one from [CWA Open Data Platform](https://opendata.cwa.gov.tw/devManual/insrtuction).
- You need to know the exact `city_name` and `town_name` for your target location.

## Tips for Memory Usage

  Due to the memory requirements of SSL (for secure connections) and handling forecast data, it’s important to optimize your configuration to avoid running out of memory.

  1. **Disable Data Access Option:**

      Disable the `data_access` option to automatically free memory by clearing fetched forecast data after publishing states. This is useful on devices with limited RAM when you do not need to retain data.

  2. **Use a Microcontroller with PSRAM:**

      Devices equipped with PSRAM are more capable of reliably handling larger data sets.

  3. **Limit Data with `time_to` and `weather_elements`:**

      Use the `time_to` option to restrict the forecast time window, and specify only the required weather elements in `weather_elements`. This reduces the amount of data fetched from the API, as well as the amount of data processed and stored in large datasets.

  4. **Minimize Unnecessary Components:**

      Avoid enabling memory-intensive components such as `web_server`. Also, only define the `sensors` and `text sensors` you actually need from this component, rather than including all available options.

## Components

### `cwa_town_forecast`

#### Configuration Variables:

* **id** (Optional, ID): The ID to use for this component.
* **time_id** (Required, ID): The ID of the `time` component used to provide the current time for forecast indexing.
* **api_key** (Required, string, templatable): Your CWA Open Data API key.
* **city_name** (Required, string, templatable): The name of the city (e.g., "新北市").
* **town_name** (Required, string, templatable): The name of the [town](https://opendata.cwa.gov.tw/opendatadoc/Opendata_City.pdf) (e.g., "中和區").
* **mode** (Required, string): Forecast range mode. Default `3-DAYS`. Options:
  * `3-DAYS` (鄉鎮天氣預報-XXX未來3天天氣預報)
  * `7-DAYS` (鄉鎮天氣預報-XXX未來1週天氣預報)
* **weather_elements** (Optional, list of strings): Forecast elements to fetch. Defaults to all available elements if not set. Limiting the number of `weather_elements` can help reduce memory usage. Options:
  * 3-DAYS Mode
    - 溫度
    - 露點溫度
    - 相對濕度
    - 體感溫度
    - 舒適度指數
    - 風速
    - 風向
    - 3小時降雨機率
    - 天氣現象
    - 天氣預報綜合描述

  * 7-DAYS Mode
    - 平均溫度
    - 最高溫度
    - 最低溫度
    - 平均露點溫度
    - 平均相對濕度
    - 最高體感溫度
    - 最低體感溫度
    - 最大舒適度指數
    - 最小舒適度指數
    - 風速
    - 風向
    - 12小時降雨機率
    - 天氣現象
    - 紫外線指數
    - 天氣預報綜合描述
* **time_to** (Optional, Time): Specify a time offset (e.g., `12h`, `1d`) to fetch forecast data for a future time window. Reducing the time range can lower memory usage.
* **fallback_to_first_element** (Optional, boolean): Whether to fallback to the first time element if no matching time is found when publishing data. Default `true`.
* **data_access** (Optional, boolean): Whether to retain fetched forecast data after publishing states. Default `false`. When disabled, data is cleared after publishing to optimize memory usage.
* **watchdog_timeout** (Optional, Time): Timeout for the request watchdog. Default `30s`.
* **http_connect_timeout** (Optional, Time): HTTP connect timeout. Default `5s`.
* **http_timeout** (Optional, Time): HTTP read timeout. Default `10s`.
* **update_interval** (Optional, Time): How often to check for new data. Defaults to `never` (manual updates only).

#### Automations

##### Automation Triggers:

*   **on_data_change** (Optional, Action): An automation action to be performed when new data is received. In Lambdas you can get the value from the trigger with `data`.

#### Sensors

##### 3-DAYS Mode

```yaml
sensor:
  - platform: cwa_town_forecast
    mode: 3-DAYS
    temperature:
      name: "Temperature"
    dew_point:
      name: "Dew Point"
    apparent_temperature:
      name: "Apparent Temperature"
    relative_humidity:
      name: "Relative Humidity"
    wind_speed:
      name: "Wind Speed"
    probability_of_precipitation:
      name: "Probability Of Precipitation (3H)"

text_sensor:
  - platform: cwa_town_forecast
    mode: 3-DAYS
    comfort_index:
      name: "Comfort Index"
    comfort_index_description:
      name: "Comfort Index Description"
    weather:
      name: "Weather"
    weather_code:
      name: "Weather Code"
    weather_description:
      name: "Weather Description"
    weather_icon:
      name: "Weather Icon"
    wind_direction:
      name: "Wind Direction"
    beaufort_scale:
      name: "Beaufort Scale"
    city:
      name: "City"
    town:
      name: "Town"
    last_updated:
      name: "Last Updated"
    last_success:
      name: "Last Success"
```

##### 7-DAYS Mode

```yaml
sensor:
  - platform: cwa_town_forecast
    mode: 7-DAYS
    temperature:
      name: "Avg Temperature"
    max_temperature:
      name: "Max Temperature"
    min_temperature:
      name: "Min Temperature"
    dew_point:
      name: "Avg Dew Point"
    relative_humidity:
      name: "Avg Relative Humidity"
    max_apparent_temperature:
      name: "Max Apparent Temperature"
    min_apparent_temperature:
      name: "Min Apparent Temperature"
    wind_speed:
      name: "Wind Speed"
    probability_of_precipitation:
      name: "Probability Of Precipitation (12H)"
    uv_index:
      name: "UV Index"

text_sensor:
  - platform: cwa_town_forecast
    mode: 7-DAYS
    max_comfort_index:
      name: "Max Comfort Index"
    min_comfort_index:
      name: "Min Comfort Index"
    max_comfort_index_description:
      name: "Max Comfort Index Description"
    min_comfort_index_description:
      name: "Min Comfort Index Description"
    wind_direction:
      name: "Wind Direction"
    beaufort_scale:
      name: "Beaufort Scale"
    weather:
      name: "Weather"
    weather_code:
      name: "Weather Code"
    weather_icon:
      name: "Weather Icon"
    uv_exposure_level:
      name: "UV Exposure Level"
    weather_description:
      name: "Weather Description"
    city:
      name: "City"
    town:
      name: "Town"
    last_updated:
      name: "Last Updated"
    last_success:
      name: "Last Success"
```

#### Example

```yaml
external_components:
  - source: github://parkghost/esphome-cwa-town-forecast
    components: [ cwa_town_forecast ]

time:
  - platform: sntp
    id: esp_time
    timezone: Asia/Taipei

cwa_town_forecast:
  - api_key: !secret cwa_api_key
    id: town_forecast_id
    city_name: 新北市
    town_name: 中和區
    mode: 3-DAYS
```

#### Use In Lambdas

```cpp
using namespace cwa_town_forecast;

auto forecast_3d_data = id(town_forecast_3d).get_data();
auto forecast_7d_data = id(town_forecast_7d).get_data();
bool fallback = true;

auto now = id(esp_time).now();
if(!now.is_valid()){
  ESP_LOGW("forecast", "Current time is not valid");
  return;
}

ESP_LOGI("forecast", "Current time: %s", now.strftime("%Y-%m-%d %H:%M:%S").c_str());

ESP_LOGI("forecast", "Current Forecast(Nearly 1-3 hours):");
ESP_LOGI("forecast", "  LocationsName: %s", forecast_3d_data.locations_name.c_str());
ESP_LOGI("forecast", "  LocationName: %s", forecast_3d_data.location_name.c_str());
ESP_LOGI("forecast", "  Temperature: %s°C", forecast_3d_data.find_value(ElementValueKey::TEMPERATURE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Dew Point: %s°C", forecast_3d_data.find_value(ElementValueKey::DEW_POINT, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Apparent Temperature: %s°C", forecast_3d_data.find_value(ElementValueKey::APPARENT_TEMPERATURE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Comfort Index: %s", forecast_3d_data.find_value(ElementValueKey::COMFORT_INDEX, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Comfort Index Description: %s", forecast_3d_data.find_value(ElementValueKey::COMFORT_INDEX_DESCRIPTION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Relative Humidity: %s%%", forecast_3d_data.find_value(ElementValueKey::RELATIVE_HUMIDITY, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Wind Direction: %s", forecast_3d_data.find_value(ElementValueKey::WIND_DIRECTION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Wind Speed: %s m/s", forecast_3d_data.find_value(ElementValueKey::WIND_SPEED, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Beaufort Scale: %s", forecast_3d_data.find_value(ElementValueKey::BEAUFORT_SCALE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Probability Of Precipitation: %s%%", forecast_3d_data.find_value(ElementValueKey::PROBABILITY_OF_PRECIPITATION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather: %s", forecast_3d_data.find_value(ElementValueKey::WEATHER, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather Description: %s", forecast_3d_data.find_value(ElementValueKey::WEATHER_DESCRIPTION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather Code: %s", forecast_3d_data.find_value(ElementValueKey::WEATHER_CODE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather Icon: %s", forecast_3d_data.find_value(ElementValueKey::WEATHER_ICON, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "");

ESP_LOGI("forecast", "Today's Forecast(From now until midnight):");
std::tm start = mktm(now.year, now.month, now.day_of_month, 0, 0, 0);
std::tm end = mktm(now.year, now.month, now.day_of_month, 23, 59, 59);
auto min_max = forecast_3d_data.find_min_max_values(ElementValueKey::TEMPERATURE, start, end);
ESP_LOGI("forecast", "  Temperature: %.0f-%.0f °C", min_max.first, min_max.second);
min_max = forecast_3d_data.find_min_max_values(ElementValueKey::APPARENT_TEMPERATURE, start, end);
ESP_LOGI("forecast", "  Apparent Temperature: %.0f-%.0f °C", min_max.first, min_max.second);
min_max = forecast_3d_data.find_min_max_values(ElementValueKey::RELATIVE_HUMIDITY, start, end);
ESP_LOGI("forecast", "  Relative Humidity: %.0f-%.0f %%", min_max.first, min_max.second);
ESP_LOGI("forecast", "");

// see https://www.cwa.gov.tw/V8/C/W/Town/Town.html?TID=6500300
ESP_LOGI("forecast", "Current Forecast(Day/Night):");
ESP_LOGI("forecast", "  LocationsName: %s", forecast_7d_data.locations_name.c_str());
ESP_LOGI("forecast", "  LocationName: %s", forecast_7d_data.location_name.c_str());
ESP_LOGI("forecast", "  Avg Temperature: %s°C", forecast_7d_data.find_value(ElementValueKey::TEMPERATURE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Avg Dew Point: %s°C", forecast_7d_data.find_value(ElementValueKey::DEW_POINT, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Avg Relative Humidity: %s%%", forecast_7d_data.find_value(ElementValueKey::RELATIVE_HUMIDITY, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Max Temperature: %s°C", forecast_7d_data.find_value(ElementValueKey::MAX_TEMPERATURE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Min Temperature: %s°C", forecast_7d_data.find_value(ElementValueKey::MIN_TEMPERATURE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Max Apparent Temperature: %s°C", forecast_7d_data.find_value(ElementValueKey::MAX_APPARENT_TEMPERATURE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Min Apparent Temperature: %s°C", forecast_7d_data.find_value(ElementValueKey::MIN_APPARENT_TEMPERATURE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Max Comfort Index: %s", forecast_7d_data.find_value(ElementValueKey::MAX_COMFORT_INDEX, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Min Comfort Index: %s", forecast_7d_data.find_value(ElementValueKey::MIN_COMFORT_INDEX, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Max Comfort Index Description: %s", forecast_7d_data.find_value(ElementValueKey::MAX_COMFORT_INDEX_DESCRIPTION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Min Comfort Index Description: %s", forecast_7d_data.find_value(ElementValueKey::MIN_COMFORT_INDEX_DESCRIPTION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Probability Of Precipitation: %s%%", forecast_7d_data.find_value(ElementValueKey::PROBABILITY_OF_PRECIPITATION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  UV Index: %s", forecast_7d_data.find_value(ElementValueKey::UV_INDEX, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  UV Exposure Level: %s", forecast_7d_data.find_value(ElementValueKey::UV_EXPOSURE_LEVEL, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather: %s", forecast_7d_data.find_value(ElementValueKey::WEATHER, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather Code: %s", forecast_7d_data.find_value(ElementValueKey::WEATHER_CODE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather Icon: %s", forecast_7d_data.find_value(ElementValueKey::WEATHER_ICON, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather Description: %s", forecast_7d_data.find_value(ElementValueKey::WEATHER_DESCRIPTION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Wind Direction: %s", forecast_7d_data.find_value(ElementValueKey::WIND_DIRECTION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Wind Speed: %s m/s", forecast_7d_data.find_value(ElementValueKey::WIND_SPEED, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Beaufort Scale: %s", forecast_7d_data.find_value(ElementValueKey::BEAUFORT_SCALE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "");

// see https://www.cwa.gov.tw/V8/C/W/Town/Town.html?TID=6500300
ESP_LOGI("forecast", "7-Day Forecast(Day/Night):");
{
  const auto &record = forecast_7d_data;
  auto now_tm = now.to_c_tm();
  for (int d = 0; d < 7; ++d) {
    // noon entry
    std::tm noon = now_tm;
    noon.tm_mday += d;
    noon.tm_hour = 12; noon.tm_min = 0; noon.tm_sec = 0;
    std::mktime(&noon);
    auto esp_noon = tm_to_esptime(noon);
    auto date_str = esp_noon.strftime("%Y-%m-%d");
    auto weekday = esp_noon.strftime("%a");
    std::string min_val = record.find_value(ElementValueKey::MIN_TEMPERATURE, false, noon);
    std::string max_val = record.find_value(ElementValueKey::MAX_TEMPERATURE, false, noon);
    std::string icon = record.find_value(ElementValueKey::WEATHER_ICON, false, noon);
    std::string rain = record.find_value(ElementValueKey::PROBABILITY_OF_PRECIPITATION, false, noon);
    ESP_LOGI("forecast", "  %s %s %s: icon %s, rain %s%%, min %s°C, max %s°C",
              date_str.c_str(),weekday.c_str(),  "(Day)  ", icon.c_str(), rain.c_str(), min_val.c_str(), max_val.c_str());

    // night entry
    std::tm night = now_tm;
    night.tm_mday += d;
    night.tm_hour = 21; night.tm_min = 0; night.tm_sec = 0;
    std::mktime(&night);
    auto esp_night = tm_to_esptime(night);
    std::string min_val2 = record.find_value(ElementValueKey::MIN_TEMPERATURE, false, night);
    std::string max_val2 = record.find_value(ElementValueKey::MAX_TEMPERATURE, false, night);
    std::string icon2 = record.find_value(ElementValueKey::WEATHER_ICON, false, night);
    std::string rain2 = record.find_value(ElementValueKey::PROBABILITY_OF_PRECIPITATION, false, night);
    ESP_LOGI("forecast", "  %s %s %s: icon %s, rain %s%%, min %s°C, max %s°C",
              date_str.c_str(), weekday.c_str(), "(Night)",icon2.c_str(), rain2.c_str(), min_val2.c_str(), max_val2.c_str());
  }
}
```
Output
```
Current time: 2025-05-08 09:04:02
Current Forecast(Nearly 1-3 hours):
  LocationsName: 新北市
  LocationName: 中和區
  Temperature: 26°C
  Dew Point: 21°C
  Apparent Temperature: 29°C
  Comfort Index: 24
  Comfort Index Description: 舒適
  Relative Humidity: 73%
  Wind Direction: 偏東風
  Wind Speed: 2 m/s
  Beaufort Scale: 2
  Probability Of Precipitation: 20%
  Weather: 陰
  Weather Description: 陰。降雨機率20%。溫度攝氏26至29度。舒適至悶熱。偏東風 平均風速1-2級(每秒2公尺)。相對濕度66至73%。
  Weather Code: 07
  Weather Icon: cloudy

Today's Forecast(From now until midnight):
  Temperature: 20-31 °C
  Apparent Temperature: 22-35 °C
  Relative Humidity: 62-84 %

Current Forecast(Day/Night):
  LocationsName: 新北市
  LocationName: 中和區
  Avg Temperature: 27°C
  Avg Dew Point: 22°C
  Avg Relative Humidity: 70%
  Max Temperature: 31°C
  Min Temperature: 20°C
  Max Apparent Temperature: 35°C
  Min Apparent Temperature: 22°C
  Max Comfort Index: 27
  Min Comfort Index: 20
  Max Comfort Index Description: 悶熱
  Min Comfort Index Description: 舒適
  Probability Of Precipitation: 20%
  UV Index: 7
  UV Exposure Level: 高量級
  Weather: 陰時多雲
  Weather Code: 06
  Weather Icon: cloudy
  Weather Description: 陰時多雲。降雨機率20%。溫度攝氏20至31度。舒適至悶熱。偏東風 風速2級(每秒2公尺)。相對濕度70%。
  Wind Direction: 偏東風
  Wind Speed: 2 m/s
  Beaufort Scale: 2

7-Day Forecast(Day/Night):
  2025-05-08 Thu (Day)  : icon cloudy, rain 20%, min 20°C, max 31°C
  2025-05-08 Thu (Night): icon night-partly-cloudy, rain 0%, min 24°C, max 28°C
  2025-05-09 Fri (Day)  : icon partly-lightning, rain 30%, min 24°C, max 33°C
  2025-05-09 Fri (Night): icon lightning-rainy, rain 50%, min 24°C, max 29°C
  2025-05-10 Sat (Day)  : icon lightning-rainy, rain 50%, min 22°C, max 25°C
  2025-05-10 Sat (Night): icon lightning-rainy, rain 60%, min 19°C, max 22°C
  2025-05-11 Sun (Day)  : icon lightning-rainy, rain -%, min 19°C, max 24°C
  2025-05-11 Sun (Night): icon night-partly-cloudy, rain -%, min 20°C, max 22°C
  2025-05-12 Mon (Day)  : icon partly-cloudy, rain -%, min 20°C, max 27°C
  2025-05-12 Mon (Night): icon night-partly-cloudy, rain -%, min 20°C, max 24°C
  2025-05-13 Tue (Day)  : icon partly-cloudy, rain -%, min 20°C, max 30°C
  2025-05-13 Tue (Night): icon night-partly-cloudy, rain -%, min 22°C, max 27°C
  2025-05-14 Wed (Day)  : icon partly-cloudy, rain -%, min 22°C, max 32°C
  2025-05-14 Wed (Night): icon night-partly-cloudy, rain -%, min 24°C, max 29°C
```

##### Displaying Weather Icons with Material Design Icons

This component automatically converts [Central Weather Administration (CWA) weather codes](https://www.cwa.gov.tw/V8/assets/pdf/Weather_Icon.pdf) to [Material Design Icons](https://pictogrammers.com/library/mdi/category/weather/). Icon names are normalized by stripping the `weather-` prefix (e.g. use `sunny` instead of `weather-sunny`). For the complete mapping table, see [Icon Mapping Table](docs/cwa_weather_code_to_mdi_icon_mapping_table.xlsx).

Download the MDI TTF here: [materialdesignicons-webfont.ttf](https://github.com/Templarian/MaterialDesign-Webfont/blob/master/fonts/materialdesignicons-webfont.ttf)

```yaml
text_sensor:
  - platform: cwa_town_forecast
    weather_icon:
      id: weather_icon
      name: "Weather Icon"

font:
  - file: "resources/fonts/materialdesignicons.ttf"
    id: icon_weather_font
    size: 60
    glyphs:
      - "\U000F0599"  # mdi:weather-sunny
      - "\U000F0595"  # mdi:weather-partly-cloudy
      - "\U000F0590"  # mdi:weather-cloudy
      - "\U000F0596"  # mdi:weather-pouring
      - "\U000F0F33"  # mdi:weather-partly-rainy
      - "\U000F0597"  # mdi:weather-rainy
      - "\U000F067E"  # mdi:weather-lightning-rainy
      - "\U000F0F32"  # mdi:weather-partly-lightning
      - "\U000F067F"  # mdi:weather-snowy-rainy
      - "\U000F0591"  # mdi:weather-fog
      - "\U000F0598"  # mdi:weather-snowy
      - "\U000F0594"  # mdi:weather-night
      - "\U000F0F31"  # mdi:weather-night-partly-cloudy

display:
  - platform: waveshare_epaper
    id: !extend my_display
    model: $display_model
    rotation: $display_rotation
    update_interval: $display_update_interval
    full_update_every: $display_full_update_every
    lambda: |-
      int width = it.get_width();
      int height = it.get_height();
      auto BLACK = Color(0, 0, 0, 0);
      auto RED = Color(255, 0, 0, 0);
      auto WHITE = Color(255, 255, 255, 0);
      if (it.get_display_type() == DisplayType::DISPLAY_TYPE_BINARY) {
        BLACK = COLOR_ON;
        WHITE = COLOR_OFF;
      } else if (it.get_display_type() == DisplayType::DISPLAY_TYPE_COLOR) {
        it.fill(WHITE);
      }

      const std::map<std::string, std::string> ICON_NAME_TO_UNICODE_MAP = {
          {"sunny","\U000F0599"},
          {"partly-cloudy","\U000F0595"},
          {"cloudy","\U000F0590"},
          {"pouring","\U000F0596"},
          {"partly-rainy","\U000F0F33"},
          {"rainy","\U000F0597"},
          {"lightning-rainy","\U000F067E"},
          {"partly-lightning","\U000F0F32"},
          {"snowy-rainy","\U000F067F"},
          {"fog","\U000F0591"},
          {"snowy","\U000F0598"},
          {"night","\U000F0594"},
          {"night-partly-cloudy","\U000F0F31"},
      };

      if (id(weather_icon).has_state()){
          auto icon_map_it = ICON_NAME_TO_UNICODE_MAP.find(id(weather_icon).state);
          std::string unicode_icon = icon_map_it != ICON_NAME_TO_UNICODE_MAP.end() ? icon_map_it->second : std::string("");

          it.printf(width / 2, height / 2, id(icon_weather_font), BLACK, TextAlign::CENTER, unicode_icon.c_str());
      }
```