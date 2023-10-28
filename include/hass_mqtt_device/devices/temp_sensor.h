/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#pragma once

#include "hass_mqtt_device/core/device_base.h"

/**
 * @brief Class for a temperature sensor device
 *
 * Derived from DeviceBase
 */

class TemperatureSensorDevice : public DeviceBase
{
public:
    /**
     * @brief Construct a new Light object
     *
     * @param device_name The name of the device
     * @param unique_id The unique id of the device
     */
    TemperatureSensorDevice(const std::string& device_name, const std::string& unique_id);

    /**
     * @brief Implement init function for this device
     */
    void init();

    /**
     * @brief Update the temperature value. Should be called by the user and be
     * kept in sync with the actual measured temperature
     *
     * @param state The new state of the light
     */
    void update(double value);

private:
};
