# Displaying Weather Icons

This component maps [Central Weather Administration (CWA) weather codes](https://www.cwa.gov.tw/V8/assets/pdf/Weather_Icon.pdf) to display-ready font glyphs with distinct day and night variants. Both icon sets below are fully supported — Weather Icons is recommended (and the default) for its finer day/night vocabulary; select with the `IconSet` parameter and pair it with the **matching font file**:

| IconSet | Unique glyphs | Font file | Source |
|---------|---------------|-----------|--------|
| `IconSet::WEATHER_ICONS` (recommended, default) | 24 (13 day / 11 night) | [weathericons-regular-webfont.ttf](https://github.com/erikflowers/weather-icons/blob/master/font/weathericons-regular-webfont.ttf) | [Weather Icons](https://erikflowers.github.io/weather-icons/) |
| `IconSet::MDI` | 13 (11 day / 2 night) | [materialdesignicons-webfont.ttf](https://github.com/Templarian/MaterialDesign-Webfont/blob/master/fonts/materialdesignicons-webfont.ttf) | [Material Design Icons](https://pictogrammers.com/library/mdi/) |

For the complete mapping table of both sets, see [Weather Icon Mapping Table](https://parkghost.github.io/esphome-cwa-town-forecast/weather_icon_mapping.html).

Call `find_weather_icon()` on the forecast data inside a display lambda. It resolves the slot's `WeatherCode`, decides day/night from sunrise/sunset at the forecast location, and returns a `WeatherIconInfo` with everything needed for rendering:

The display lambda below is identical for both icon sets — only the font file and the `IconSet` argument of `find_weather_icon()` differ (see the two sections that follow):

```yaml
display:
  - platform: epaper_spi
    id: !extend my_display
    model: $display_model
    rotation: $display_rotation
    update_interval: $display_update_interval
    full_update_every: $display_full_update_every
    lambda: |-
      using namespace cwa_town_forecast;
      int width = it.get_width();
      int height = it.get_height();
      auto BLACK = Color(0, 0, 0, 0);
      auto RED = Color(255, 0, 0, 0);
      auto WHITE = Color(255, 255, 255, 0);
      if (it.get_display_type() == DisplayType::DISPLAY_TYPE_COLOR) {
        it.fill(WHITE);
      }

      auto now = id(esp_time).now();
      if (!now.is_valid()) return;
      const auto &data = id(town_forecast_7d).get_data();
      auto wi = data.find_weather_icon(false, now.to_c_tm());
      if (wi.valid) {
        it.printf(width / 2, height / 2, id(icon_weather_font),
                  weather_code_highlight(wi.code) ? RED : BLACK,
                  TextAlign::CENTER, wi.unicode);
      }
```

## Using Weather Icons

```yaml
font:
  - file: "resources/fonts/weathericons-regular-webfont.ttf"
    id: icon_weather_font
    size: 60
    glyphs:
      - "\uF00D"  # wi-day-sunny
      - "\uF00C"  # wi-day-sunny-overcast
      - "\uF002"  # wi-day-cloudy
      - "\uF013"  # wi-cloudy
      - "\uF009"  # wi-day-showers
      - "\uF00B"  # wi-day-sprinkle
      - "\uF008"  # wi-day-rain
      - "\uF00E"  # wi-day-storm-showers
      - "\uF010"  # wi-day-thunderstorm
      - "\uF0B2"  # wi-day-sleet
      - "\uF003"  # wi-day-fog
      - "\uF014"  # wi-fog
      - "\uF00A"  # wi-day-snow
      - "\uF02E"  # wi-night-clear
      - "\uF081"  # wi-night-alt-partly-cloudy
      - "\uF086"  # wi-night-alt-cloudy
      - "\uF029"  # wi-night-alt-showers
      - "\uF02B"  # wi-night-alt-sprinkle
      - "\uF028"  # wi-night-alt-rain
      - "\uF02C"  # wi-night-alt-storm-showers
      - "\uF02D"  # wi-night-alt-thunderstorm
      - "\uF0B4"  # wi-night-alt-sleet
      - "\uF04A"  # wi-night-fog
      - "\uF02A"  # wi-night-alt-snow
```

```cpp
auto wi = data.find_weather_icon(false, now.to_c_tm());  // IconSet::WEATHER_ICONS is the default
```

## Using Material Design Icons

The glyph codepoints belong to whichever font you load, so the icon set and the font must always
change together:

```yaml
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
```

```cpp
auto wi = data.find_weather_icon(false, now.to_c_tm(), IconSet::MDI);
```

Sizing note: at the same nominal font size, Weather Icons glyphs render ~1.45x the size while MDI glyphs render ~0.97x — when switching sets, scale the font size by ~0.69 (e.g. MDI at 140 ≈ Weather Icons at 96) to keep the same visual footprint.

## `WeatherIconInfo` and helpers

`WeatherIconInfo` carries `code` (the CWA WeatherCode), `name` (e.g. `wi-day-sunny`), `unicode` (UTF-8 glyph for `printf`), `flags` (`WEATHER_FLAG_*` condition bits), `valid`, and condition helpers `is_rainy()` / `is_lightning()` / `is_foggy()` / `is_snowy()`.

Related helpers:

- `Record::is_daytime(tm)` — sunrise/sunset test at the forecast location.
- `find_weather_icon_name(code, is_day, set)` / `find_weather_icon_unicode(code, is_day, set)` — pure lookups; `set` defaults to `IconSet::WEATHER_ICONS`, pass `IconSet::MDI` for Material Design Icons glyphs.
- `weather_code_flags(code)` / `weather_code_highlight(code)` — condition bits (rain/thunder/fog/snow) and a ready-made "emphasize this weather" flag.

Deprecated: the `weather_icon` **text sensor** is deprecated and will be removed in a future release — configuring it logs a warning at build time and the entity is now disabled by default in Home Assistant. Use `find_weather_icon()` (with either icon set) instead.

See also: [Lambda API](lambda-api.md).
