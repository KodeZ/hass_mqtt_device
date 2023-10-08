/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#include "hass_mqtt_device/devices/on_off_light.h"
#include "hass_mqtt_device/core/function_base.h"
#include "hass_mqtt_device/functions/on_off_light.h"
#include <functional>
#include <memory>

/**
 * @brief Implements the light device
 *
 * Derived from DeviceBase
 */

OnOffLightDevice::OnOffLightDevice(const std::string &deviceName,
                                   const std::string &unique_id,
                                   std::function<void(bool)> setStateCallback)
    : DeviceBase(deviceName, unique_id),
      m_set_state_callback(setStateCallback) {}

void OnOffLightDevice::init() {
  std::shared_ptr<OnOffLightFunction> on_off_light =
      std::make_shared<OnOffLightFunction>("on_off_light", m_set_state_callback);
  std::shared_ptr<FunctionBase> on_off_light_base = on_off_light;
  registerFunction(on_off_light_base);
}

void OnOffLightDevice::setState(bool state) {
  std::shared_ptr<OnOffLightFunction> on_off_light =
      std::dynamic_pointer_cast<OnOffLightFunction>(
          findFunction("on_off_light"));
  on_off_light->setState(state);
}
