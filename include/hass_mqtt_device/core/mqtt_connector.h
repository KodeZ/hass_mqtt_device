/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#pragma once

#include <string>
#include <vector>
#include "device_base.h" // Assuming you have a base class for devices

/**
 * @brief Class for connecting to an MQTT server and registering devices to listen for their MQTT topics
 * 
 * @note This class is not thread-safe, so it should only be used from one thread
 */

class MQTTConnector {
public:
    /**
     * @brief Construct a new MQTTConnector object
     * 
     * @param server The MQTT server to connect to
     * @param username The username to use when connecting to the MQTT server
     * @param password The password to use when connecting to the MQTT server
     */
    MQTTConnector(const std::string& server, const std::string& username, const std::string& password);

    /**
     * @brief Connect to the MQTT server
     */
    bool connect();

    /**
     * @brief Disconnect from the MQTT server
     */
    void disconnect();

    /**
     * @brief Check if connected to the MQTT server
     * 
     * @return true if connected to the MQTT server, false otherwise
     */
    bool isConnected() const;

    /**
     * @brief Register a device to listen for its MQTT topics
     * 
     * @param device The device to register
     */
    void registerDevice(DeviceBase& device);

    /**
     * @brief Process incoming MQTT messages
     * 
     * @note This method should be called in the main loop
     */
    void processMessages();

private:
    std::string m_server;
    std::string m_username;
    std::string m_password;
    std::vector<DeviceBase*> m_registeredDevices; // List of registered devices
};
