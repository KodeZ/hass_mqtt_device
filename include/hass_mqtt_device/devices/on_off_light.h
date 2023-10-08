/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#pragma once

#include "hass_mqtt_device/core/device_base.h"

/**
 * @brief Class for a light device
 *
 * Derived from DeviceBase
 */

class OnOffLightDevice : public DeviceBase {
public:
  /**
   * @brief Construct a new Light object
   *
   * @param deviceName The name of the device
   * @param unique_id The unique id of the device
   * @param setStateCallback The callback to call when the state of the light
   */
  OnOffLightDevice(const std::string &deviceName, const std::string &unique_id,
                   std::function<void(bool)> setStateCallback);

  /**
   * @brief Destroy the Light object
   */
  ~OnOffLightDevice() = default;

  /**
   * @brief Implement init function for this device
   */
  void init();

  /**
   * @brief Update the state of the light. Should be called by the user and be
   * kept in sync with the actual state of the light
   *
   * @param state The new state of the light
   */
  void setState(bool state);

private:
  std::function<void(bool)> m_set_state_callback;
};
