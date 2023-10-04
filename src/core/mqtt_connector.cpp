/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#include "hass_mqtt_device/core/mqtt_connector.h"
#include "hass_mqtt_device/logger/logger.hpp"
#include <spdlog/spdlog.h>
#include <mqtt/async_client.h>

MQTTConnector::MQTTConnector(const std::string& server, const std::string& username, const std::string& password)
    : m_server(server), m_username(username), m_password(password) {
    // Create an instance of the async MQTT client
    m_client = std::make_unique<mqtt::async_client>(m_server, ""); // unique client ID

    // Set up connection options
    m_connOpts = std::make_unique<mqtt::connect_options>();
    m_connOpts->set_user_name(m_username);
    m_connOpts->set_password(m_password);
}

bool MQTTConnector::connect() {
    LOG_INFO("Connecting to MQTT server: {}", m_server);
    try {
        auto response = m_client->connect(*m_connOpts)->wait();
        return response.get_reason_code() == mqtt::reason_code::SUCCESS;
    } catch (const mqtt::exception& exc) {
        LOG_ERROR("Error connecting to MQTT server: {}", exc.what());
        return false;
    }
}

void MQTTConnector::disconnect() {
    LOG_INFO("Disconnecting from MQTT server: {}", m_server);
    try {
        m_client->disconnect()->wait();
    } catch (const mqtt::exception& exc) {
        LOG_ERROR("Error disconnecting from MQTT server: {}", exc.what());
    }
}

bool MQTTConnector::isConnected() const {
    return m_client->is_connected();
}

void MQTTConnector::registerDevice(DeviceBase& device) {
    LOG_INFO("Registering device: {}", device.getName());
    m_registeredDevices.push_back(&device);
    // Subscribe to the device's topic(s) here if needed
    // m_client->subscribe(device.getTopic(), QOS_LEVEL);
}

void MQTTConnector::processMessages() {
    // This method might be used to periodically check for messages or handle other tasks.
    // With Paho's async client, incoming messages would typically be handled by a callback.
}
