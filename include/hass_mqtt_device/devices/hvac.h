/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#pragma once

#include "hass_mqtt_device/core/device_base.h"
#include "hass_mqtt_device/functions/hvac.h"

/**
 * @brief Class for a hvac device
 *
 * Derived from DeviceBase
 */

class HvacDevice : public DeviceBase
{
public:
    /**
     * @brief Construct a new hvac device
     *
     * @param device_name The name of the device
     * @param unique_id The unique id of the device
     */
    HvacDevice(const std::string& device_name, const std::string& unique_id = "");

    /**
     * @brief Implement init function for this device
     *
     * @param control_cb The callback to call when a set command is received
     * @param supported_features The supported features of the hvac
     * @param device_modes The supported device modes of the hvac
     * @param fan_modes The supported fan modes of the hvac
     * @param swing_modes The supported swing modes of the hvac
     * @param preset_modes The supported preset modes of the hvac
     */
    void init(std::function<void(HvacSupportedFeatures, std::string)> control_cb,
              unsigned supported_features,
              std::vector<std::string> device_modes = {},
              std::vector<std::string> fan_modes = {},
              std::vector<std::string> swing_modes = {},
              std::vector<std::string> preset_modes = {});

    /**
     * @brief Get the function object
     *
     * Returning the shared pointer to the function so that the user can update
     * the states of the function.
     *
     * @return std::shared_ptr<HvacFunction> pointing to the hvac function
     */
    std::shared_ptr<HvacFunction> getFunction();

private:
};
