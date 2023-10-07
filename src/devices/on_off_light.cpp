/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#include "hass_mqtt_device/devices/on_off_light.h"
#include "hass_mqtt_device/core/function_base.h"
#include "hass_mqtt_device/functions/on_off_light.h"
#include <memory>

/**
 * @brief Implements the light device
 *
 * Derived from DeviceBase
 */

OnOffLightDevice::OnOffLightDevice(const std::string &deviceName,
                                   const std::string &unique_id)
    : DeviceBase(deviceName, unique_id) {
  std::shared_ptr<OnOffLightFunction> on_off_light =
      std::make_shared<OnOffLightFunction>("on_off_light");
  std::shared_ptr<FunctionBase> on_off_light_base = on_off_light;
  registerFunction(on_off_light_base);
}
