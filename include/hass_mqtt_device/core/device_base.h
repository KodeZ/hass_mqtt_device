/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#pragma once

#include <string>

/**
 * @brief Base class for all devices that can be registered with the
 * MQTTConnector.
 *
 * This class provides the basic interface that all devices should implement.
 * Derived classes should provide specific implementations for their device
 * type.
 */

class DeviceBase {
public:
  /**
   * @brief Construct a new DeviceBase object
   *
   * @param deviceName The name of the device
   */
  DeviceBase(const std::string &deviceName);

  /**
   * @brief Destroy the DeviceBase object
   */
  virtual ~DeviceBase() = default;

  /**
   * @brief Get the MQTT topic for this device
   *
   * @return The MQTT topic for this device
   */
  virtual std::string getTopic() const = 0;

  /**
   * @brief Get the name of this device
   *
   * @return The name of this device
   */
  std::string getName() const;

  /**
   * @brief Process an incoming MQTT message
   *
   * @param topic The topic of the incoming message. This will be concatenated
   * with the device name and base topic
   * @param payload The payload of the incoming message
   */
  virtual void processMQTTMessage(const std::string &topic,
                                  const std::string &payload) = 0;

  /**
   * @brief Publish an MQTT message
   *
   * @param topic The topic to publish to. This will be concatenated with the
   * device name and base topic
   * @param payload The payload to publish
   */
  virtual void publishMessage(const std::string &topic,
                              const std::string &payload) = 0;

protected:
  std::string m_deviceName;
  std::string m_baseTopic;

  // Add any additional protected members or helper methods as needed
};
