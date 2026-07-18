# Lambda API

Direct access to forecast data from ESPHome lambdas via `get_data()`. The snippets below assume the
two component instances from the [README example](../README.md#example) (`town_forecast_3d` / `town_forecast_7d`).


```cpp
using namespace cwa_town_forecast;

const auto &data_3d = id(town_forecast_3d).get_data();
const auto &data_7d = id(town_forecast_7d).get_data();
bool fallback = true;

auto now = id(esp_time).now();
if(!now.is_valid()){
  ESP_LOGW("forecast", "Current time is not valid");
  return;
}

ESP_LOGI("forecast", "Current time: %s", now.strftime("%Y-%m-%d %H:%M:%S").c_str());

ESP_LOGI("forecast", "Current Forecast(Nearly 1-3 hours):");
ESP_LOGI("forecast", "  LocationsName: %s", data_3d.locations_name.c_str());
ESP_LOGI("forecast", "  LocationName: %s", data_3d.location_name.c_str());
ESP_LOGI("forecast", "  Temperature: %s°C", data_3d.find_value(ElementValueKey::TEMPERATURE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Dew Point: %s°C", data_3d.find_value(ElementValueKey::DEW_POINT, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Apparent Temperature: %s°C", data_3d.find_value(ElementValueKey::APPARENT_TEMPERATURE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Comfort Index: %s", data_3d.find_value(ElementValueKey::COMFORT_INDEX, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Comfort Index Description: %s", data_3d.find_value(ElementValueKey::COMFORT_INDEX_DESCRIPTION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Relative Humidity: %s%%", data_3d.find_value(ElementValueKey::RELATIVE_HUMIDITY, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Wind Direction: %s", data_3d.find_value(ElementValueKey::WIND_DIRECTION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Wind Speed: %s m/s", data_3d.find_value(ElementValueKey::WIND_SPEED, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Beaufort Scale: %s", data_3d.find_value(ElementValueKey::BEAUFORT_SCALE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Probability Of Precipitation: %s%%", data_3d.find_value(ElementValueKey::PROBABILITY_OF_PRECIPITATION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather: %s", data_3d.find_value(ElementValueKey::WEATHER, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather Description: %s", data_3d.find_value(ElementValueKey::WEATHER_DESCRIPTION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather Code: %s", data_3d.find_value(ElementValueKey::WEATHER_CODE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather Icon: %s", data_3d.find_weather_icon(fallback, now.to_c_tm()).name);
ESP_LOGI("forecast", "");

ESP_LOGI("forecast", "Today's Forecast(From now until midnight):");
std::tm start = mktm(now.year, now.month, now.day_of_month, 0, 0, 0);
std::tm end = mktm(now.year, now.month, now.day_of_month, 23, 59, 59);
auto min_max = data_3d.find_min_max_values(ElementValueKey::TEMPERATURE, start, end);
ESP_LOGI("forecast", "  Temperature: %.0f-%.0f °C", min_max.first, min_max.second);
min_max = data_3d.find_min_max_values(ElementValueKey::APPARENT_TEMPERATURE, start, end);
ESP_LOGI("forecast", "  Apparent Temperature: %.0f-%.0f °C", min_max.first, min_max.second);
min_max = data_3d.find_min_max_values(ElementValueKey::RELATIVE_HUMIDITY, start, end);
ESP_LOGI("forecast", "  Relative Humidity: %.0f-%.0f %%", min_max.first, min_max.second);
ESP_LOGI("forecast", "");

// see https://www.cwa.gov.tw/V8/C/W/Town/Town.html?TID=6500300
ESP_LOGI("forecast", "Current Forecast(Day/Night):");
ESP_LOGI("forecast", "  LocationsName: %s", data_7d.locations_name.c_str());
ESP_LOGI("forecast", "  LocationName: %s", data_7d.location_name.c_str());
ESP_LOGI("forecast", "  Avg Temperature: %s°C", data_7d.find_value(ElementValueKey::TEMPERATURE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Avg Dew Point: %s°C", data_7d.find_value(ElementValueKey::DEW_POINT, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Avg Relative Humidity: %s%%", data_7d.find_value(ElementValueKey::RELATIVE_HUMIDITY, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Max Temperature: %s°C", data_7d.find_value(ElementValueKey::MAX_TEMPERATURE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Min Temperature: %s°C", data_7d.find_value(ElementValueKey::MIN_TEMPERATURE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Max Apparent Temperature: %s°C", data_7d.find_value(ElementValueKey::MAX_APPARENT_TEMPERATURE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Min Apparent Temperature: %s°C", data_7d.find_value(ElementValueKey::MIN_APPARENT_TEMPERATURE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Max Comfort Index: %s", data_7d.find_value(ElementValueKey::MAX_COMFORT_INDEX, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Min Comfort Index: %s", data_7d.find_value(ElementValueKey::MIN_COMFORT_INDEX, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Max Comfort Index Description: %s", data_7d.find_value(ElementValueKey::MAX_COMFORT_INDEX_DESCRIPTION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Min Comfort Index Description: %s", data_7d.find_value(ElementValueKey::MIN_COMFORT_INDEX_DESCRIPTION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Probability Of Precipitation: %s%%", data_7d.find_value(ElementValueKey::PROBABILITY_OF_PRECIPITATION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  UV Index: %s", data_7d.find_value(ElementValueKey::UV_INDEX, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  UV Exposure Level: %s", data_7d.find_value(ElementValueKey::UV_EXPOSURE_LEVEL, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather: %s", data_7d.find_value(ElementValueKey::WEATHER, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather Code: %s", data_7d.find_value(ElementValueKey::WEATHER_CODE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Weather Icon: %s", data_7d.find_weather_icon(fallback, now.to_c_tm()).name);
ESP_LOGI("forecast", "  Weather Description: %s", data_7d.find_value(ElementValueKey::WEATHER_DESCRIPTION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Wind Direction: %s", data_7d.find_value(ElementValueKey::WIND_DIRECTION, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Wind Speed: %s m/s", data_7d.find_value(ElementValueKey::WIND_SPEED, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "  Beaufort Scale: %s", data_7d.find_value(ElementValueKey::BEAUFORT_SCALE, fallback, now.to_c_tm()).c_str());
ESP_LOGI("forecast", "");

// see https://www.cwa.gov.tw/V8/C/W/Town/Town.html?TID=6500300
ESP_LOGI("forecast", "7-Day Forecast(Day/Night):");
{
  auto now_tm = now.to_c_tm();
  for (int d = 0; d < 7; ++d) {
    // daytime
    std::tm noon = now_tm;
    noon.tm_mday += d;
    noon.tm_hour = 12; noon.tm_min = 0; noon.tm_sec = 0;
    std::mktime(&noon);
    auto esp_noon = tm_to_esptime(noon);
    auto date_str = esp_noon.strftime("%Y-%m-%d");
    auto weekday = esp_noon.strftime("%a");
    std::string min_val = data_7d.find_value(ElementValueKey::MIN_TEMPERATURE, false, noon);
    std::string max_val = data_7d.find_value(ElementValueKey::MAX_TEMPERATURE, false, noon);
    auto icon = data_7d.find_weather_icon(false, noon);
    std::string rain = data_7d.find_value(ElementValueKey::PROBABILITY_OF_PRECIPITATION, false, noon);
    ESP_LOGI("forecast", "  %s %s %s: icon %s, rain %s%%, min %s°C, max %s°C",
              date_str.c_str(),weekday.c_str(),  "(Day)  ", icon.name, rain.c_str(), min_val.c_str(), max_val.c_str());

    // nighttime
    std::tm night = now_tm;
    night.tm_mday += d;
    night.tm_hour = 21; night.tm_min = 0; night.tm_sec = 0;
    std::mktime(&night);
    auto esp_night = tm_to_esptime(night);
    std::string min_val2 = data_7d.find_value(ElementValueKey::MIN_TEMPERATURE, false, night);
    std::string max_val2 = data_7d.find_value(ElementValueKey::MAX_TEMPERATURE, false, night);
    auto icon2 = data_7d.find_weather_icon(false, night);
    std::string rain2 = data_7d.find_value(ElementValueKey::PROBABILITY_OF_PRECIPITATION, false, night);
    ESP_LOGI("forecast", "  %s %s %s: icon %s, rain %s%%, min %s°C, max %s°C",
              date_str.c_str(), weekday.c_str(), "(Night)",icon2.name, rain2.c_str(), min_val2.c_str(), max_val2.c_str());
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
  Weather Icon: wi-cloudy

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
  Weather Icon: wi-cloudy
  Weather Description: 陰時多雲。降雨機率20%。溫度攝氏20至31度。舒適至悶熱。偏東風 風速2級(每秒2公尺)。相對濕度70%。
  Wind Direction: 偏東風
  Wind Speed: 2 m/s
  Beaufort Scale: 2

7-Day Forecast(Day/Night):
  2025-05-08 Thu (Day)  : icon wi-cloudy, rain 20%, min 20°C, max 31°C
  2025-05-08 Thu (Night): icon wi-night-alt-partly-cloudy, rain 0%, min 24°C, max 28°C
  2025-05-09 Fri (Day)  : icon wi-day-storm-showers, rain 30%, min 24°C, max 33°C
  2025-05-09 Fri (Night): icon wi-night-alt-storm-showers, rain 50%, min 24°C, max 29°C
  2025-05-10 Sat (Day)  : icon wi-day-storm-showers, rain 50%, min 22°C, max 25°C
  2025-05-10 Sat (Night): icon wi-night-alt-storm-showers, rain 60%, min 19°C, max 22°C
  2025-05-11 Sun (Day)  : icon wi-day-storm-showers, rain -%, min 19°C, max 24°C
  2025-05-11 Sun (Night): icon wi-night-alt-partly-cloudy, rain -%, min 20°C, max 22°C
  2025-05-12 Mon (Day)  : icon wi-day-sunny-overcast, rain -%, min 20°C, max 27°C
  2025-05-12 Mon (Night): icon wi-night-alt-partly-cloudy, rain -%, min 20°C, max 24°C
  2025-05-13 Tue (Day)  : icon wi-day-sunny-overcast, rain -%, min 20°C, max 30°C
  2025-05-13 Tue (Night): icon wi-night-alt-partly-cloudy, rain -%, min 22°C, max 27°C
  2025-05-14 Wed (Day)  : icon wi-day-sunny-overcast, rain -%, min 22°C, max 32°C
  2025-05-14 Wed (Night): icon wi-night-alt-partly-cloudy, rain -%, min 24°C, max 29°C
```

## Weather Elements and Weather Element Values

### 3-DAYS [Reference Source](../resources/town_forecast_api_3d_simplified.json)

| ElementName       | ElementValue Key      | Example Value   |
|-------------------|-----------------------|-----------------|
| 溫度              | TEMPERATURE           | 22              |
| 露點溫度          | DEW_POINT             | 19              |
| 相對濕度          | RELATIVE_HUMIDITY     | 85              |
| 體感溫度          | APPARENT_TEMPERATURE  | 24              |
| 舒適度指數        | COMFORT_INDEX         | 21              |
| 舒適度指數        | COMFORT_INDEX_DESCRIPTION | 舒適         |
| 風速              | WIND_SPEED            | 2               |
| 風速              | BEAUFORT_SCALE        | 2               |
| 風向              | WIND_DIRECTION        | 東北風          |
| 3小時降雨機率     | PROBABILITY_OF_PRECIPITATION | 80      |
| 天氣現象          | WEATHER               | 短暫陣雨        |
| 天氣現象          | WEATHER_CODE          | 08              |
| 天氣現象          | WEATHER_ICON (deprecated) | pouring     |
| 天氣預報綜合描述  | WEATHER_DESCRIPTION   | 短暫陣雨。降雨機率80%。溫度攝氏22度。舒適。東北風 平均風速1-2級(每秒2公尺)。相對濕度83至85%。 |

### 7-DAYS [Reference Source](../resources/town_forecast_api_7d_simplified.json)

| ElementName         | ElementValue Key              | Example Value   |
|---------------------|-------------------------------|-----------------|
| 平均溫度            | TEMPERATURE                   | 28              |
| 最高溫度            | MAX_TEMPERATURE               | 30              |
| 最低溫度            | MIN_TEMPERATURE               | 26              |
| 平均露點溫度        | DEW_POINT                     | 21              |
| 平均相對濕度        | RELATIVE_HUMIDITY             | 65              |
| 最高體感溫度        | MAX_APPARENT_TEMPERATURE      | 31              |
| 最低體感溫度        | MIN_APPARENT_TEMPERATURE      | 27              |
| 最大舒適度指數      | MAX_COMFORT_INDEX             | 26              |
| 最大舒適度指數      | MAX_COMFORT_INDEX_DESCRIPTION | 舒適            |
| 最小舒適度指數      | MIN_COMFORT_INDEX             | 24              |
| 最小舒適度指數      | MIN_COMFORT_INDEX_DESCRIPTION | 舒適            |
| 風速                | WIND_SPEED                    | 5               |
| 風速                | BEAUFORT_SCALE                | 3               |
| 風向                | WIND_DIRECTION                | 東南風          |
| 12小時降雨機率      | PROBABILITY_OF_PRECIPITATION  | 0               |
| 紫外線指數          | UV_INDEX                      | 8               |
| 紫外線曝曬等級      | UV_EXPOSURE_LEVEL             | 過量級          |
| 天氣現象            | WEATHER                       | 晴時多雲        |
| 天氣現象            | WEATHER_CODE                  | 02              |
| 天氣現象            | WEATHER_ICON (deprecated)     | partly-cloudy   |
| 天氣預報綜合描述    | WEATHER_DESCRIPTION           | 晴時多雲。降雨機率0%。溫度攝氏26至30度。舒適。東南風 風速3級(每秒5公尺)。相對濕度65%。 |

See also: [Displaying Weather Icons](weather-icons.md).
