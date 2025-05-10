import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TIME_ID
from esphome.components import time
from esphome import automation

DEPENDENCIES = ["network", "time"]
AUTO_LOAD = ["json", "watchdog", "sensor", "text_sensor"]

cwa_town_forecast_ns = cg.esphome_ns.namespace("cwa_town_forecast")
CWATownForecast = cwa_town_forecast_ns.class_("CWATownForecast", cg.PollingComponent)
CWATownForecastRecord = cwa_town_forecast_ns.struct("Record")
CWATownForecastRecordRef = CWATownForecastRecord.operator("ref")

CWATownForecastMode = cwa_town_forecast_ns.enum("Mode")
CWATownForecastClearCacheEarly = cwa_town_forecast_ns.enum("ClearCacheEarly")

MODE_THREE_DAYS = "3-DAYS"
MODE_SEVEN_DAYS = "7-DAYS"
Mode = {
    MODE_THREE_DAYS: CWATownForecastMode.ThreeDays,
    MODE_SEVEN_DAYS: CWATownForecastMode.SevenDays,
}

CLEAR_CACHE_EARLY_AUTO = "AUTO"
CLEAR_CACHE_EARLY_ON = "ON"
CLEAR_CACHE_EARLY_OFF = "OFF"
ClearCacheEarly = {
    CLEAR_CACHE_EARLY_AUTO: CWATownForecastClearCacheEarly.Auto,
    CLEAR_CACHE_EARLY_ON: CWATownForecastClearCacheEarly.On,
    CLEAR_CACHE_EARLY_OFF: CWATownForecastClearCacheEarly.Off,
}

CONF_CWA_TOWN_FORECAST_ID = "cwa_town_forecast_id"

CONF_API_KEY = "api_key"
CONF_CITY_NAME = "city_name"
CONF_TOWN_NAME = "town_name"
CONF_TIME_TO = "time_to"
CONF_MODE = "mode"
CONF_WEATHER_ELEMENTS = "weather_elements"
CONF_FALLBACK_TO_FIRST_ELEMENT = "fallback_to_first_element"
CONF_SENSOR_EXPIRY = "sensor_expiry"
CONF_DATA_ACCESS = "data_access"
CONF_CLEAR_CACHE_EARLY = "clear_cache_early"
CONF_ON_DATA_CHANGE = "on_data_change"

CONF_WATCHDOG_TIMEOUT = "watchdog_timeout"
CONF_HTTP_CONNECT_TIMEOUT = "http_connect_timeout"
CONF_HTTP_TIMEOUT = "http_timeout"

CHILD_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_CWA_TOWN_FORECAST_ID): cv.use_id(CWATownForecast),
    }
)

CITY_NAMES = [
    "宜蘭縣",
    "桃園市",
    "新竹縣",
    "苗栗縣",
    "彰化縣",
    "南投縣",
    "雲林縣",
    "嘉義縣",
    "屏東縣",
    "臺東縣",
    "花蓮縣",
    "澎湖縣",
    "基隆市",
    "新竹市",
    "嘉義市",
    "臺北市",
    "高雄市",
    "新北市",
    "臺中市",
    "臺南市",
    "連江縣",
    "金門縣",
]

WEATHER_ELEMENT_NAMES_3DAYS = [
    "溫度",
    "露點溫度",
    "相對濕度",
    "體感溫度",
    "舒適度指數",
    "風速",
    "風向",
    "3小時降雨機率",
    "天氣現象",
    "天氣預報綜合描述",
]

WEATHER_ELEMENT_NAMES_7DAYS = [
    "平均溫度",
    "最高溫度",
    "最低溫度",
    "平均露點溫度",
    "平均相對濕度",
    "最高體感溫度",
    "最低體感溫度",
    "最大舒適度指數",
    "最小舒適度指數",
    "風速",
    "風向",
    "12小時降雨機率",
    "天氣現象",
    "紫外線指數",
    "天氣預報綜合描述",
]


def validate_mode_weather_elements(config):
    mode = config.get(CONF_MODE)
    if mode == MODE_THREE_DAYS:
        allowed = WEATHER_ELEMENT_NAMES_3DAYS
    elif mode == MODE_SEVEN_DAYS:
        allowed = WEATHER_ELEMENT_NAMES_7DAYS
    else:
        raise cv.Invalid(f"Unknown mode: {mode}")

    for elem in config.get(CONF_WEATHER_ELEMENTS, []):
        if elem not in allowed:
            raise cv.Invalid(f"Weather element '{elem}' is not allowed for mode {mode}")
    return config


CONFIG_SCHEMA = cv.All(
    cv.ensure_list(
        cv.Schema(
            {
                cv.GenerateID(): cv.declare_id(CWATownForecast),
                cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
                cv.Optional(CONF_API_KEY, default=""): cv.templatable(cv.string),
                cv.Optional(CONF_CITY_NAME, default=""): cv.templatable(
                    cv.one_of(*CITY_NAMES)
                ),
                cv.Optional(CONF_TOWN_NAME, default=""): cv.templatable(cv.string),
                cv.Required(CONF_MODE): cv.enum(Mode, upper=True),
                cv.Optional(CONF_WEATHER_ELEMENTS, default=[]): cv.ensure_list(
                    cv.string
                ),
                cv.Optional(CONF_TIME_TO): cv.templatable(
                    cv.All(
                        cv.positive_not_null_time_period,
                        cv.positive_time_period_milliseconds,
                    )
                ),
                cv.Optional(
                    CONF_FALLBACK_TO_FIRST_ELEMENT, default=True
                ): cv.templatable(cv.boolean),
                cv.Optional(CONF_SENSOR_EXPIRY, default="1h"): cv.templatable(
                    cv.All(
                        cv.positive_not_null_time_period,
                        cv.positive_time_period_milliseconds,
                    )
                ),
                cv.Optional(CONF_DATA_ACCESS, default=False): cv.templatable(cv.boolean),
                cv.Optional(CONF_CLEAR_CACHE_EARLY, default=CLEAR_CACHE_EARLY_AUTO): cv.templatable(cv.enum(ClearCacheEarly, upper=True)),
                cv.Optional(CONF_ON_DATA_CHANGE): automation.validate_automation(),
                cv.Optional(CONF_WATCHDOG_TIMEOUT, default="30s"): cv.templatable(
                    cv.All(
                        cv.positive_not_null_time_period,
                        cv.positive_time_period_milliseconds,
                    )
                ),
                cv.Optional(CONF_HTTP_CONNECT_TIMEOUT, default="5s"): cv.templatable(
                    cv.All(
                        cv.positive_not_null_time_period,
                        cv.positive_time_period_milliseconds,
                    )
                ),
                cv.Optional(CONF_HTTP_TIMEOUT, default="10s"): cv.templatable(
                    cv.All(
                        cv.positive_not_null_time_period,
                        cv.positive_time_period_milliseconds,
                    )
                ),
            }
        )
        .add_extra(validate_mode_weather_elements)
        .extend(cv.polling_component_schema("never")),
    ),
    cv.only_on_esp32,
    cv.only_with_arduino,
)


async def to_code(configs):
    for config in configs:
        var = cg.new_Pvariable(config[CONF_ID])
        await cg.register_component(var, config)

        if CONF_TIME_ID in config:
            rtc = await cg.get_variable(config[CONF_TIME_ID])
            cg.add(var.set_time(rtc))
        if CONF_API_KEY in config:
            api_key = await cg.templatable(config[CONF_API_KEY], [], cg.std_string)
            cg.add(var.set_api_key(api_key))
        if CONF_CITY_NAME in config:
            city_name = await cg.templatable(config[CONF_CITY_NAME], [], cg.std_string)
            cg.add(var.set_city_name(city_name))
        if CONF_TOWN_NAME in config:
            town_name = await cg.templatable(config[CONF_TOWN_NAME], [], cg.std_string)
            cg.add(var.set_town_name(town_name))
        if CONF_MODE in config:
            cg.add(var.set_mode(config[CONF_MODE]))
        for weather_element in config[CONF_WEATHER_ELEMENTS]:
            cg.add(var.add_weather_element(weather_element))
        if CONF_TIME_TO in config:
            time_to = await cg.templatable(config[CONF_TIME_TO], [], cg.uint32)
            cg.add(var.set_time_to(time_to))
        if CONF_FALLBACK_TO_FIRST_ELEMENT in config:
            fallback = await cg.templatable(
                config[CONF_FALLBACK_TO_FIRST_ELEMENT], [], cg.bool_
            )
            cg.add(var.set_fallback_to_first_element(fallback))
        if CONF_SENSOR_EXPIRY in config:
            sensor_expiry = await cg.templatable(config[CONF_SENSOR_EXPIRY], [], cg.uint32)
            cg.add(var.set_sensor_expiry(sensor_expiry))
        if CONF_DATA_ACCESS in config:
            access = await cg.templatable(config[CONF_DATA_ACCESS], [], cg.bool_)
            cg.add(var.set_data_access(access))
        if CONF_CLEAR_CACHE_EARLY in config:
            clear_cache_early = await cg.templatable(config[CONF_CLEAR_CACHE_EARLY], [], cg.std_string)
            cg.add(var.set_clear_cache_early(clear_cache_early))
        for trigger in config.get(CONF_ON_DATA_CHANGE, []):
            await automation.build_automation(
                var.get_on_data_change_trigger(),
                [(CWATownForecastRecordRef, "data")],
                trigger,
            )
        if CONF_WATCHDOG_TIMEOUT in config:
            timeout = await cg.templatable(config[CONF_WATCHDOG_TIMEOUT], [], cg.uint32)
            cg.add(var.set_watchdog_timeout(timeout))
        if CONF_HTTP_CONNECT_TIMEOUT in config:
            timeout = await cg.templatable(
                config[CONF_HTTP_CONNECT_TIMEOUT], [], cg.uint32
            )
            cg.add(var.set_http_connect_timeout(timeout))
        if CONF_HTTP_TIMEOUT in config:
            timeout = await cg.templatable(config[CONF_HTTP_TIMEOUT], [], cg.uint32)
            cg.add(var.set_http_timeout(timeout))

    cg.add_library("WiFiClientSecure", None)
    cg.add_library("HTTPClient", None)
    cg.add_library("UrlEncode", None)
    cg.add_library("sunset", None)
