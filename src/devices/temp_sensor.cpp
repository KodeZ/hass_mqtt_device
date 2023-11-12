/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#include "hass_mqtt_device/devices/temp_sensor.h"
#include "hass_mqtt_device/core/function_base.h"
#include "hass_mqtt_device/functions/sensor.h"
#include "hass_mqtt_device/functions/sensor_attributes_factory.hpp"
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

void TemperatureSensorDevice::init(std::string function_name)
{
    LOG_DEBUG("Initializing temperature sensor device with function name: " + function_name);
    m_function_name = function_name;
    SensorAttributes attributes = getTemperatureSensorAttributes();

    std::shared_ptr<SensorFunction<double>> temperature =
        std::make_shared<SensorFunction<double>>(function_name, attributes);
    std::shared_ptr<FunctionBase> temperature_base = temperature;
    registerFunction(temperature_base);
}

void TemperatureSensorDevice::update(double value)
{
    std::shared_ptr<SensorFunction<double>> temperature =
        std::dynamic_pointer_cast<SensorFunction<double>>(findFunction(m_function_name));
    if(temperature)
    {
        temperature->update(value);
    }
    else
    {
        LOG_ERROR("Could not find temperature function");
    }
}
