/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#include "hass_mqtt_device/devices/temp_sensor.h"
#include "hass_mqtt_device/core/function_base.h"
#include "hass_mqtt_device/functions/sensor.h"
#include "hass_mqtt_device/logger/logger.hpp"
#include <functional>
#include <memory>

/**
 * @brief Implements the light device
 *
 * Derived from DeviceBase
 */

TemperatureSensorDevice::TemperatureSensorDevice(const std::string& device_name,
                                   const std::string& unique_id)
    : DeviceBase(device_name, unique_id)
{
}

void TemperatureSensorDevice::init()
{
    SensorAttributes attributes;
    attributes.unit_of_measurement = "°C";
    attributes.device_class = "temperature";
    attributes.state_class = "measurement";
    attributes.suggested_display_precision = 1;

    std::shared_ptr<SensorFunction<double>> temperature =
        std::make_shared<SensorFunction<double>>("temperature", attributes);
    std::shared_ptr<FunctionBase> temperature_base = temperature;
    registerFunction(temperature_base);
}

void TemperatureSensorDevice::update(double value)
{
    std::shared_ptr<SensorFunction<double>> temperature =
        std::dynamic_pointer_cast<SensorFunction<double>>(findFunction("temperature"));
    if(temperature)
    {
        temperature->update(value);
    }
    else
    {
        LOG_ERROR("Could not find temperature function");
    }
}
