/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#include "hass_mqtt_device/devices/hvac.h"
#include "hass_mqtt_device/core/function_base.h"
#include "hass_mqtt_device/functions/hvac.h"
#include "hass_mqtt_device/logger/logger.hpp"
#include <functional>
#include <memory>
#include <utility>

/**
 * @brief Implements the hvac device
 *
 * Derived from DeviceBase
 */

HvacDevice::HvacDevice(const std::string& device_name,
                       const std::string& unique_id)

    : DeviceBase(device_name, unique_id)
{
}

void HvacDevice::init(std::function<void(HvacSupportedFeatures, std::string)> control_cb,
                      unsigned supported_features,
                      std::vector<std::string> device_modes,
                      std::vector<std::string> fan_modes,
                      std::vector<std::string> swing_modes,
                      std::vector<std::string> preset_modes)
{
    std::shared_ptr<HvacFunction> sw = std::make_shared<HvacFunction>("hvac",
                                                                      control_cb,
                                                                      supported_features,
                                                                      device_modes,
                                                                      fan_modes,
                                                                      swing_modes,
                                                                      preset_modes);
    std::shared_ptr<FunctionBase> sw_base = sw;
    registerFunction(sw_base);
}

std::shared_ptr<HvacFunction> HvacDevice::getFunction()
{
    std::shared_ptr<HvacFunction> sw = std::dynamic_pointer_cast<HvacFunction>(findFunction("hvac"));
    if(sw)
    {
        return sw;
    }
    LOG_ERROR("Could not find hvac function");
    return nullptr;
}
