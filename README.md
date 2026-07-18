# ESPHome CWA Town Forecast Component

This external component for ESPHome fetches [town weather forecast](https://www.cwa.gov.tw/V8/C/K/CommonFaq/forecast_all.html) data in either **3-DAYS** or **7-DAYS** from Taiwan’s Central Weather Administration (CWA) Open Data platform.

> For a complete example using this component with an e-paper display, see the [Weather Panel YAML](https://github.com/parkghost/esphome-epaper-examples/blob/main/weather-panel.yaml) in the [esphome-epaper-examples](https://github.com/parkghost/esphome-epaper-examples) repository.

![weather-panel.yaml](https://raw.githubusercontent.com/parkghost/esphome-epaper-examples/main/images/weather-panel.jpg)

## Prerequisites

- You need a CWA Open Data API authorization key. Obtain one from [CWA Open Data Platform](https://opendata.cwa.gov.tw/devManual/insrtuction).
- You need to know the exact `city_name` and `town_name` for your target location.

## Tips for Memory Usage

  Due to the memory requirements of SSL (for secure connections) and handling forecast data, it’s important to optimize your configuration to avoid running out of memory.

  1. **Use a Microcontroller with PSRAM:**

      Devices equipped with PSRAM are more capable of reliably handling larger data sets.

  2. **Disable `retain_fetched_data`:**

      Disable the `retain_fetched_data` option to automatically free memory by clearing fetched forecast data after publishing states. This is useful on devices with limited RAM when you do not need to retain data.

  3. **Limit Data with `time_to` and `weather_elements`:**

      Use the `time_to` option to restrict the forecast time window, and specify only the required weather elements in `weather_elements`. This reduces the amount of data fetched from the API, as well as the amount of data processed and stored in large datasets.

  4. **Minimize Unnecessary Components:**

      Avoid enabling memory-intensive components such as `web_server`. Also, only define the `sensors` and `text sensors` you actually need from this component, rather than including all available options.

## Components

### `cwa_town_forecast`

#### Configuration Variables:

* **id** (Optional, ID): The ID to use for this component.
* **time_id** (Required, ID): The ID of the `time` component used to provide the current time for forecast indexing.
* **http_request_id** (Optional, ID): The ID of the `http_request` component. Automatically detected if only one exists.
* **api_key** (Required, string, templatable): Your CWA Open Data API key.
* **city_name** (Required, string, templatable): The name of the city (e.g., "新北市").
* **town_name** (Required, string, templatable): The name of the [town](https://opendata.cwa.gov.tw/opendatadoc/Opendata_City.pdf) (e.g., "中和區").
* **mode** (Required, string): Forecast range mode. Options:
  * `3-DAYS`: [e.g. 鄉鎮天氣預報-新北市未來3天天氣預報](https://opendata.cwa.gov.tw/dataset/all/F-D0047-069)
  * `7-DAYS`: [e.g. 鄉鎮天氣預報-新北市未來1週天氣預報](https://opendata.cwa.gov.tw/dataset/all/F-D0047-071)
* **weather_elements** (Optional, list of strings): Forecast elements to fetch. Defaults to all available elements if not set. Limiting the number of `weather_elements` can help reduce memory usage. Options:
  * 3-DAYS Mode
    - `溫度`
    - `露點溫度`
    - `相對濕度`
    - `體感溫度`
    - `舒適度指數`
    - `風速`
    - `風向`
    - `3小時降雨機率`
    - `天氣現象`
    - `天氣預報綜合描述`

  * 7-DAYS Mode
    - `平均溫度`
    - `最高溫度`
    - `最低溫度`
    - `平均露點溫度`
    - `平均相對濕度`
    - `最高體感溫度`
    - `最低體感溫度`
    - `最大舒適度指數`
    - `最小舒適度指數`
    - `風速`
    - `風向`
    - `12小時降雨機率`
    - `天氣現象`
    - `紫外線指數`
    - `天氣預報綜合描述`
* **time_to** (Optional, Time, templatable): Specify a time offset (e.g., `2h`, `1d`) to fetch forecast data for a future time window. Reducing the time range can lower memory usage.
* **early_data_clear** (Optional, string): Whether to clear data early before fetching new data to optimize memory usage. Default `AUTO`. Options:
  - `AUTO`: Never clear data early when PSRAM is present.
  - `ON`: Clear data early(Reduces heap pressure and fragmentation to optimize memory usage. Side effect: data is empty on fetch failure).
  - `OFF`: Never clear data early.
* **fallback_to_first_element** (Optional, boolean, templatable): Whether to fallback to the first time element if no matching time is found when publishing data. Default `true`.
* **retain_fetched_data** (Optional, boolean, templatable): Whether to retain fetched forecast data after publishing states. Default `false`. When disabled, data is cleared after publishing to optimize memory usage.
* **sensor_expiry** (Optional, Time, templatable): Duration to retain last values after failures. Default `1h`.
* **retry_count** (Optional, integer, templatable): Number of retry attempts for failed HTTP requests. Default `1`. Range: 0-5.
* **retry_delay** (Optional, Time, templatable): Base delay between retry attempts. Uses exponential backoff with jitter. Default `1s`.
* **update_interval** (Optional, Time): How often to check for new data. Defaults to `never` (manual updates only).

#### Automations

##### Automation Triggers:

*   **on_data_change** (Optional, Action): An automation action to be performed when new data is received. In Lambdas you can get the value from the trigger with `data`.
*   **on_error** (Optional, Action): An automation action to be performed when a fetch error occurs.

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
    last_error:
      name: "Last Error"
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
    last_error:
      name: "Last Error"
```

#### Example

```yaml
esp32:
  board: esp32-s3-devkitc-1
  framework:
    type: esp-idf

external_components:
  - source: github://parkghost/esphome-cwa-town-forecast
    components: [cwa_town_forecast]

http_request:
  timeout: 10s
  watchdog_timeout: 30s
  verify_ssl: false

time:
  - platform: sntp
    id: esp_time
    timezone: Asia/Taipei

cwa_town_forecast:
  - api_key: !secret cwa_api_key
    id: town_forecast_3d
    city_name: 新北市
    town_name: 中和區
    mode: 3-DAYS

  - api_key: !secret cwa_api_key
    id: town_forecast_7d
    city_name: 新北市
    town_name: 中和區
    mode: 7-DAYS
```

#### Use In Lambdas

Forecast data is directly accessible from lambdas — match values to a point in time, scan ranges,
or iterate raw time slots:

```cpp
using namespace cwa_town_forecast;
const auto &data = id(town_forecast_7d).get_data();
std::string temp = data.find_value(ElementValueKey::TEMPERATURE, true, id(esp_time).now().to_c_tm());
```

Full guide with runnable examples and the complete element value key reference:
[Lambda API](docs/lambda-api.md)

#### Displaying Weather Icons

CWA weather codes map to day/night font glyphs in two icon sets —
[Weather Icons](https://erikflowers.github.io/weather-icons/) (recommended, default) or
Material Design Icons (`IconSet::MDI`) — paired with the matching font file:

```cpp
auto wi = data.find_weather_icon(false, now.to_c_tm());
it.printf(x, y, id(icon_weather_font), TextAlign::CENTER, wi.unicode);
```

Font setup, glyph lists, day/night rules and sizing notes: [Weather Icons guide](docs/weather-icons.md) ·
interactive mapping table: [Weather Icon Mapping Table](https://parkghost.github.io/esphome-cwa-town-forecast/weather_icon_mapping.html)
