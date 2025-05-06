import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    STATE_CLASS_MEASUREMENT,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_WIND_SPEED,
)

from . import CONF_CWA_TOWN_FORECAST_ID, CHILD_SCHEMA, CWATownForecast

DEPENDENCIES = ["cwa_town_forecast"]

CONF_TEMPERATURE = "temperature"
CONF_DEW_POINT = "dew_point"
CONF_APPARENT_TEMPERATURE = "apparent_temperature"
CONF_RELATIVE_HUMIDITY = "relative_humidity"
CONF_WIND_SPEED = "wind_speed"
CONF_PROBABILITY_OF_PRECIPITATION = "probability_of_precipitation"
CONF_WEATHER_CODE = "weather_code"
# new 7 days forecast sensors
CONF_MAX_TEMPERATURE = "max_temperature"
CONF_MIN_TEMPERATURE = "min_temperature"
CONF_MAX_APPARENT_TEMPERATURE = "max_apparent_temperature"
CONF_MIN_APPARENT_TEMPERATURE = "min_apparent_temperature"
CONF_UV_INDEX = "uv_index"

SENSORS_3DAYS = [
    CONF_TEMPERATURE,
    CONF_DEW_POINT,
    CONF_APPARENT_TEMPERATURE,
    CONF_RELATIVE_HUMIDITY,
    CONF_WIND_SPEED,
    CONF_PROBABILITY_OF_PRECIPITATION,
]

SENSORS_7DAYS = [
    CONF_TEMPERATURE,
    CONF_MAX_TEMPERATURE,
    CONF_MIN_TEMPERATURE,
    CONF_DEW_POINT,
    CONF_RELATIVE_HUMIDITY,
    CONF_MAX_APPARENT_TEMPERATURE,
    CONF_MIN_APPARENT_TEMPERATURE,
    CONF_WIND_SPEED,
    CONF_PROBABILITY_OF_PRECIPITATION,
    CONF_UV_INDEX,
]

SENSORS = list(set(SENSORS_3DAYS + SENSORS_7DAYS))

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_CWA_TOWN_FORECAST_ID): cv.use_id(CWATownForecast),
        cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement="°C",
            icon="mdi:thermometer",
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_DEW_POINT): sensor.sensor_schema(
            unit_of_measurement="°C",
            icon="mdi:thermometer-plus",
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_APPARENT_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement="°C",
            icon="mdi:thermometer-lines",
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_RELATIVE_HUMIDITY): sensor.sensor_schema(
            unit_of_measurement="%",
            icon="mdi:water-percent",
            device_class=DEVICE_CLASS_HUMIDITY,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_WIND_SPEED): sensor.sensor_schema(
            unit_of_measurement="m/s",
            icon="mdi:weather-windy",
            device_class=DEVICE_CLASS_WIND_SPEED,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_PROBABILITY_OF_PRECIPITATION): sensor.sensor_schema(
            unit_of_measurement="%",
            icon="mdi:weather-rainy",
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=0,
        ),
        # new 7 days forecast sensors
        cv.Optional(CONF_MAX_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement="°C",
            icon="mdi:thermometer",
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_MIN_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement="°C",
            icon="mdi:thermometer",
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_MAX_APPARENT_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement="°C",
            icon="mdi:thermometer-lines",
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_MIN_APPARENT_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement="°C",
            icon="mdi:thermometer-lines",
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_UV_INDEX): sensor.sensor_schema(
            icon="mdi:weather-sunny",
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=0,
        ),
    }
).extend(CHILD_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_CWA_TOWN_FORECAST_ID])

    for key in SENSORS:
        if sens_config := config.get(key):
            sens = await sensor.new_sensor(sens_config)
            cg.add(getattr(parent, f"set_{key}_sensor")(sens))
