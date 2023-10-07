// Include the corresponding header file
#include "hass_mqtt_device/core/mqtt_connector.h"

// Include any other necessary headers
#include "hass_mqtt_device/utils/logger.hpp" // For logging

// Constructor implementation
MQTTConnector::MQTTConnector(const std::string& server, const std::string& username, const std::string& password)
    : m_server(server), m_username(username), m_password(password) {
    LOG_DEBUG("MQTTConnector created with server: {}", server);

    // Initialize the MQTT library
    mosquitto_lib_init();
}

// Connect to the MQTT server
bool MQTTConnector::connect() {
    LOG_DEBUG("Connecting to MQTT server: {}", m_server);

    m_mosquitto = mosquitto_new(nullptr, true, nullptr);
    if (!m_mosquitto) {
        LOG_ERROR("Failed to create mosquitto instance");
        return false;
    }
    mosquitto_username_pw_set(m_mosquitto, m_username.c_str(), m_password.c_str());
    int rc = mosquitto_connect(m_mosquitto, m_server.c_str(), 1883, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        LOG_ERROR("Failed to connect to MQTT server: {}", mosquitto_strerror(rc));
        return false;
    }
    LOG_DEBUG("Connected to MQTT server: {}", m_server);

    // Subscribe to the topics of the registered devices
    for (auto& device : m_registeredDevices) {
        LOG_DEBUG("Subscribing to topic: {}", device->getTopic());
        rc = mosquitto_subscribe(m_mosquitto, nullptr, device->getTopic().c_str(), 0);
        if (rc != MOSQ_ERR_SUCCESS) {
            LOG_ERROR("Failed to subscribe to topic: {}", mosquitto_strerror(rc));
            return false;
        }
    }
    return true;
}

// Disconnect from the MQTT server
void MQTTConnector::disconnect() {
    LOG_DEBUG("Disconnecting from MQTT server: {}", m_server);
    mosquitto_disconnect(m_mosquitto);
}

// Check if connected to the MQTT server
bool MQTTConnector::isConnected() const {
    return mosquitto_is_connected(m_mosquitto);
}

// Register a device to listen for its MQTT topics
void MQTTConnector::registerDevice(std::shared_ptr<DeviceBase> device) {
    m_registeredDevices.push_back(device);
    LOG_DEBUG("Device registered");
}

// Process incoming MQTT messages
void MQTTConnector::processMessages() {
    // TODO: Implement the message processing logic
    LOG_DEBUG("Processing MQTT messages");
}

// Add any other necessary method implementations...
