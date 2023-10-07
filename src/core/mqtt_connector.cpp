// Include the corresponding header file
#include "hass_mqtt_device/core/mqtt_connector.h"
#include "hass_mqtt_device/core/device_base.h"

// Include any other necessary headers
#include "hass_mqtt_device/logger/logger.hpp" // For logging

// Constructor implementation
MQTTConnector::MQTTConnector(const std::string &server,
                             const std::string &username,
                             const std::string &password)
    : m_server(server), m_username(username), m_password(password) {
  LOG_DEBUG("MQTTConnector created with server: {}", server);

  // Initialize the MQTT library
  mosquitto_lib_init();
}

// Connect to the MQTT server
bool MQTTConnector::connect() {
  LOG_DEBUG("Connecting to MQTT server: {}", m_server);

  m_mosquitto = mosquitto_new(nullptr, true, this);
  if (!m_mosquitto) {
    LOG_ERROR("Failed to create mosquitto instance");
    return false;
  }
  mosquitto_username_pw_set(m_mosquitto, m_username.c_str(),
                            m_password.c_str());
  int rc = mosquitto_connect(m_mosquitto, m_server.c_str(), 1883, 60);
  if (rc != MOSQ_ERR_SUCCESS) {
    LOG_ERROR("Failed to connect to MQTT server: {}", mosquitto_strerror(rc));
    return false;
  }
  LOG_DEBUG("Connected to MQTT server: {}", m_server);

  return true;
}

// Disconnect from the MQTT server
void MQTTConnector::disconnect() {
  LOG_DEBUG("Disconnecting from MQTT server: {}", m_server);
  mosquitto_disconnect(m_mosquitto);
}

// Check if connected to the MQTT server
bool MQTTConnector::isConnected() const { return m_is_connected; }

// Register a device to listen for its MQTT topics
void MQTTConnector::registerDevice(std::shared_ptr<DeviceBase> device) {
  device->setParentConnector(shared_from_this());
  m_registered_devices.push_back(device);

  // If connected, subscribe to the topic
  if (m_is_connected) {
    for (auto &topic : device->getSubscribeTopics()) {
      LOG_DEBUG("Subscribing to topic: {}", topic);
      int rc = mosquitto_subscribe(m_mosquitto, nullptr, topic.c_str(), 0);
      if (rc != MOSQ_ERR_SUCCESS) {
        LOG_ERROR("Failed to subscribe to topic: {}", mosquitto_strerror(rc));
        return;
      }
    }
  }
  LOG_DEBUG("Device registered");
}

// Process incoming MQTT messages
void MQTTConnector::processMessages(int timeout) {
  LOG_DEBUG("Processing MQTT messages");
  int rc = mosquitto_loop(m_mosquitto, timeout, 1);
  if (rc != MOSQ_ERR_SUCCESS && rc != MOSQ_ERR_NO_CONN) {
    LOG_ERROR("Failed to process MQTT messages: {}", mosquitto_strerror(rc));
  }
}

// Callback for incoming MQTT messages, implementing the on_message
void MQTTConnector::messageCallback(mosquitto *mosq, void *obj,
                                    const mosquitto_message *message) {
  LOG_DEBUG("Received MQTT message on topic: {}", message->topic);
  MQTTConnector *connector = static_cast<MQTTConnector *>(obj);
  // Convert the topic and message to a string
  std::string topic(message->topic);
  std::string payload(static_cast<char *>(message->payload),
                      message->payloadlen);

  for (auto &device : connector->m_registered_devices) {
    // Check if the topic starts with the device's topic
    if (std::string(message->topic).find(device->getId()) == 0) {
      // Call the device's onMessage method
      device->processMessage(topic, payload);
    }
  }
}

// Callback for successful connection to the MQTT server, implementing
// on_connect
void MQTTConnector::connectCallback(mosquitto *mosq, void *obj, int rc) {
  LOG_DEBUG("Connected to MQTT server");
  MQTTConnector *connector = static_cast<MQTTConnector *>(obj);

  // Subscribe to the topics of the registered devices
  for (auto &device : connector->m_registered_devices) {
    for (auto &topic : device->getSubscribeTopics()) {
      LOG_DEBUG("Subscribing to topic: {}", topic);
      rc = mosquitto_subscribe(connector->m_mosquitto, nullptr, topic.c_str(),
                               0);
      if (rc != MOSQ_ERR_SUCCESS) {
        LOG_ERROR("Failed to subscribe to topic: {}", mosquitto_strerror(rc));
        return;
      }
    }
  }

  connector->m_is_connected = true;
}

// Callback for disconnection from the MQTT server, implementing
// on_disconnect
void MQTTConnector::disconnectCallback(mosquitto *mosq, void *obj, int rc) {
  LOG_INFO("Disconnected from MQTT server");
  MQTTConnector *connector = static_cast<MQTTConnector *>(obj);
  connector->m_is_connected = false;
}

// Callback for successful subscription to an MQTT topic, implementing
// on_subscribe
void MQTTConnector::subscribeCallback(mosquitto *mosq, void *obj, int mid,
                                      int qos_count, const int *granted_qos) {
  LOG_DEBUG("Subscribed to MQTT topic");
}

// Callback for successful unsubscription to an MQTT topic, implementing
// on_unsubscribe
void MQTTConnector::unsubscribeCallback(mosquitto *mosq, void *obj, int mid) {
  LOG_ERROR("Unsubscribed from MQTT topic");
}
