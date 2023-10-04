#pragma once

#include <string>

/**
 * @brief Base class for all devices that can be registered with the MQTTConnector.
 * 
 * This class provides the basic interface that all devices should implement.
 * Derived classes should provide specific implementations for their device type.
 */

class DeviceBase {
public:
    // Constructor
    DeviceBase(const std::string& deviceName);

    // Virtual destructor to ensure derived classes are properly destructed
    virtual ~DeviceBase() = default;

    // Get the name of the device
    std::string getName() const;

    // Process a received MQTT message for this device
    virtual void processMQTTMessage(const std::string& topic, const std::string& payload) = 0;

    // Publish a message to a specific topic
    virtual void publishMessage(const std::string& topic, const std::string& payload) = 0;

protected:
    std::string m_deviceName;

    // Add any additional protected members or helper methods as needed
};
