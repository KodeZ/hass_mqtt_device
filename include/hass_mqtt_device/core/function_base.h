/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#pragma once

#include "hass_mqtt_device/core/device_base.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

/**
 * @brief This class describes a function that can be registered with a device.
 * Some devices can have multiple functions, e.g. a controller for several
 * lights.
 *
 * This class provides the basic interface that all functions should implement.
 * Derived classes should provide specific implementations for their function
 * type.
 */

class FunctionBase {
public:
  /**
   * @brief Construct a new FunctionBase object
   *
   * @param parentDevice Reference to the parent device
   * @param functionName The name of the function
   */
  FunctionBase(const std::string &functionName);

  /**
   * @brief Destroy the FunctionBase object
   */
  virtual ~FunctionBase() = default;

  /**
   * @brief Initialize the object
   *
   * Called when the function is registered with the device
   */
  virtual void init() = 0;

  /**
   * @brief Get the name of this function
   *
   * @return The name of this function
   */
  std::string getName() const;

  /**
   * @brief Get the MQTT topics to subscribe to for this function
   *
   * @return The MQTT topic for this function
   */
  virtual std::vector<std::string> getSubscribeTopics() const = 0;

  /**
   * @brief Get the discovery topic for this function
   *
   * @return The discovery topic for this function
   */
  virtual std::string getDiscoveryTopic() const = 0;

  /**
   * @brief Get the discovery payload for this function
   *
   * @return The discovery payload for this function
   */
  virtual json getDiscoveryJson() const = 0;

  /**
   * @brief Process an incoming MQTT message
   *
   * @param topic The topic of the incoming message. This will be concatenated
   * with the device name and base topic
   * @param payload The payload of the incoming message
   */
  virtual void processMessage(const std::string &topic,
                              const std::string &payload) = 0;

protected:
  std::string m_functionName;
  std::weak_ptr<DeviceBase> m_parentDevice;

private:
  friend void
  DeviceBase::registerFunction(std::shared_ptr<FunctionBase> function);
  void setParentDevice(std::weak_ptr<DeviceBase> parentDevice) {
    m_parentDevice = parentDevice;
    init();
  };
};
