import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor

from . import (
    CONF_CWA_TOWN_FORECAST_ID,
    CHILD_SCHEMA,
    CONF_MODE,
    MODE_THREE_DAYS,
    MODE_SEVEN_DAYS,
)

CONF_CITY = "city"
CONF_TOWN = "town"
CONF_LAST_UPDATED = "last_updated"
CONF_LAST_SUCCESS = "last_success"
CONF_LAST_ERROR = "last_error"
CONF_COMFORT_INDEX_DESCRIPTION = "comfort_index_description"
CONF_WEATHER = "weather"
CONF_WEATHER_CODE = "weather_code"
CONF_WEATHER_DESCRIPTION = "weather_description"
CONF_WEATHER_ICON = "weather_icon"
CONF_COMFORT_INDEX = "comfort_index"
CONF_WIND_DIRECTION = "wind_direction"
CONF_BEAUFORT_SCALE = "beaufort_scale"
# new 7 days forecast text sensors
CONF_MAX_COMFORT_INDEX = "max_comfort_index"
CONF_MIN_COMFORT_INDEX = "min_comfort_index"
CONF_MAX_COMFORT_INDEX_DESCRIPTION = "max_comfort_index_description"
CONF_MIN_COMFORT_INDEX_DESCRIPTION = "min_comfort_index_description"
CONF_UV_EXPOSURE_LEVEL = "uv_exposure_level"


TEXT_SENSORS_3DAYS = [
    CONF_COMFORT_INDEX_DESCRIPTION,
    CONF_WEATHER,
    CONF_WEATHER_CODE,
    CONF_WEATHER_DESCRIPTION,
    CONF_WEATHER_ICON,
    CONF_COMFORT_INDEX,
    CONF_WIND_DIRECTION,
    CONF_BEAUFORT_SCALE,
]

TEXT_SENSORS_7DAYS = [
    CONF_MAX_COMFORT_INDEX,
    CONF_MIN_COMFORT_INDEX,
    CONF_MAX_COMFORT_INDEX_DESCRIPTION,
    CONF_MIN_COMFORT_INDEX_DESCRIPTION,
    CONF_UV_EXPOSURE_LEVEL,
    CONF_WEATHER,
    CONF_WEATHER_CODE,
    CONF_WEATHER_DESCRIPTION,
    CONF_WEATHER_ICON,
    CONF_WIND_DIRECTION,
    CONF_BEAUFORT_SCALE,
]

TEXT_SENSORS = list(
    set(
        TEXT_SENSORS_3DAYS
        + TEXT_SENSORS_7DAYS
        + [CONF_LAST_UPDATED, CONF_LAST_SUCCESS, CONF_LAST_ERROR, CONF_CITY, CONF_TOWN]
    )
)

DIAGNOSTIC_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_LAST_UPDATED): text_sensor.text_sensor_schema(
            icon="mdi:calendar-clock",
            entity_category="diagnostic",
        ),
        cv.Optional(CONF_LAST_SUCCESS): text_sensor.text_sensor_schema(
            icon="mdi:calendar-clock",
            entity_category="diagnostic",
        ),
        cv.Optional(CONF_LAST_ERROR): text_sensor.text_sensor_schema(
            icon="mdi:calendar-clock",
            entity_category="diagnostic",
        ),
        cv.Optional(CONF_CITY): text_sensor.text_sensor_schema(
            entity_category="diagnostic"
        ),
        cv.Optional(CONF_TOWN): text_sensor.text_sensor_schema(
            entity_category="diagnostic"
        ),
    }
)

CONFIG_SCHEMA = cv.All(
    cv.typed_schema(
        {
            MODE_THREE_DAYS: cv.Schema(
                {
                    cv.Optional(
                        CONF_COMFORT_INDEX_DESCRIPTION
                    ): text_sensor.text_sensor_schema(
                        icon="mdi:thermometer",
                    ),
                    cv.Optional(CONF_WEATHER): text_sensor.text_sensor_schema(
                        icon="mdi:weather-partly-cloudy",
                    ),
                    cv.Optional(CONF_WEATHER_CODE): text_sensor.text_sensor_schema(),
                    cv.Optional(
                        CONF_WEATHER_DESCRIPTION
                    ): text_sensor.text_sensor_schema(),
                    cv.Optional(CONF_WEATHER_ICON): text_sensor.text_sensor_schema(),
                    cv.Optional(CONF_COMFORT_INDEX): text_sensor.text_sensor_schema(),
                    cv.Optional(CONF_WIND_DIRECTION): text_sensor.text_sensor_schema(),
                    cv.Optional(CONF_BEAUFORT_SCALE): text_sensor.text_sensor_schema(),
                }
            )
            .extend(DIAGNOSTIC_SCHEMA)
            .extend(CHILD_SCHEMA),
            MODE_SEVEN_DAYS: cv.Schema(
                {
                    cv.Optional(CONF_WEATHER): text_sensor.text_sensor_schema(
                        icon="mdi:weather-partly-cloudy",
                    ),
                    cv.Optional(CONF_WEATHER_CODE): text_sensor.text_sensor_schema(),
                    cv.Optional(
                        CONF_WEATHER_DESCRIPTION
                    ): text_sensor.text_sensor_schema(),
                    cv.Optional(CONF_WEATHER_ICON): text_sensor.text_sensor_schema(),
                    cv.Optional(CONF_WIND_DIRECTION): text_sensor.text_sensor_schema(),
                    cv.Optional(CONF_BEAUFORT_SCALE): text_sensor.text_sensor_schema(),
                    cv.Optional(
                        CONF_MAX_COMFORT_INDEX
                    ): text_sensor.text_sensor_schema(),
                    cv.Optional(
                        CONF_MIN_COMFORT_INDEX
                    ): text_sensor.text_sensor_schema(),
                    cv.Optional(
                        CONF_MAX_COMFORT_INDEX_DESCRIPTION
                    ): text_sensor.text_sensor_schema(),
                    cv.Optional(
                        CONF_MIN_COMFORT_INDEX_DESCRIPTION
                    ): text_sensor.text_sensor_schema(),
                    cv.Optional(
                        CONF_UV_EXPOSURE_LEVEL
                    ): text_sensor.text_sensor_schema(),
                    cv.Optional(CONF_CITY): text_sensor.text_sensor_schema(
                        entity_category="diagnostic"
                    ),
                    cv.Optional(CONF_TOWN): text_sensor.text_sensor_schema(
                        entity_category="diagnostic"
                    ),
                }
            )
            .extend(DIAGNOSTIC_SCHEMA)
            .extend(CHILD_SCHEMA),
        },
        key=CONF_MODE,
    )
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_CWA_TOWN_FORECAST_ID])

    for key in TEXT_SENSORS:
        if sens_config := config.get(key):
            sens = await text_sensor.new_text_sensor(sens_config)
            cg.add(getattr(parent, f"set_{key}_text_sensor")(sens))
