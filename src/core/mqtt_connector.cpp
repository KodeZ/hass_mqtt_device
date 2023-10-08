// Include the corresponding header file
#include "hass_mqtt_device/core/mqtt_connector.h"
#include "hass_mqtt_device/core/device_base.h"

// Include any other necessary headers
#include "hass_mqtt_device/logger/logger.hpp" // For logging
#include <array>
#include <chrono>
#include <mosquitto.h>
#include <string>
#include <thread>

constexpr std::array<int, 8> backoff_ladder = {1000, 1000,  5000,  5000,
                                               5000, 15000, 30000, 30000};
int backoff_state = 0;

// Constructor implementation
MQTTConnector::MQTTConnector(const std::string &server, const int port,
                             const std::string &username,
                             const std::string &password)
    : m_server(server), m_port(port), m_username(username),
      m_password(password) {
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

  // Set the callbacks
  mosquitto_connect_callback_set(m_mosquitto, connectCallback);
  mosquitto_disconnect_callback_set(m_mosquitto, disconnectCallback);
  mosquitto_subscribe_callback_set(m_mosquitto, subscribeCallback);
  mosquitto_unsubscribe_callback_set(m_mosquitto, unsubscribeCallback);
  mosquitto_message_callback_set(m_mosquitto, messageCallback);

  mosquitto_username_pw_set(m_mosquitto, m_username.c_str(),
                            m_password.c_str());
  int rc = mosquitto_connect(m_mosquitto, m_server.c_str(), m_port, 60);
  if (rc != MOSQ_ERR_SUCCESS) {
    LOG_ERROR("Failed to connect to MQTT server: {}", mosquitto_strerror(rc));
    return false;
  }
  LOG_DEBUG("Connected to MQTT server: {}", m_server);
  m_is_connected = true;

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
  if (!isConnected()) {
    LOG_DEBUG("Not connected to MQTT server. Attempting to reconnect.");
    static int slept_for = 0;
    LOG_DEBUG("Slept since last reconnect try {}ms", slept_for);
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
    slept_for += timeout;
    if (slept_for < backoff_ladder[backoff_state]) {
      return;
    } else {
      slept_for = 0;
      backoff_state++;
      if (backoff_state >= backoff_ladder.size()) {
        backoff_state = backoff_ladder.size() - 1;
      }

      bool rc = connect();
      if (!rc) {
        LOG_ERROR("Failed to reconnect to MQTT server: {}. Continuing to sleep "
                  "and retry.",
                  mosquitto_strerror(rc));
        return;
      }
    }
    backoff_state = 0;
  }

  // At this point, we are connected to the MQTT server
  int rc = mosquitto_loop(m_mosquitto, timeout, 1);
  if (rc != MOSQ_ERR_SUCCESS && rc != MOSQ_ERR_NO_CONN) {
    LOG_ERROR("Failed to process MQTT messages: {}", mosquitto_strerror(rc));
  }
}

// Publish a message
void MQTTConnector::publishMessage(const std::string &topic,
                                   const json &payload) {
  std::string payload_str = payload.dump();
  LOG_DEBUG("Publishing MQTT message to topic: {}", topic);
  LOG_DEBUG("MQTT message payload: {}", payload_str);
  int rc = mosquitto_publish(m_mosquitto, nullptr, topic.c_str(),
                             payload_str.size(), payload_str.c_str(), 0, false);
  if (rc != MOSQ_ERR_SUCCESS) {
    LOG_ERROR("Failed to publish MQTT message: {}", mosquitto_strerror(rc));
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
    // Check if the topic starts with the device's topic after home/
    if (topic.find("home/" + device->getId()) == 0) {
      // Call the device's processMessage method
      device->processMessage(topic, payload);
    }
  }
}

// Callback for successful connection to the MQTT server, implementing
// on_connect
void MQTTConnector::connectCallback(mosquitto *mosq, void *obj, int rc) {
  LOG_DEBUG("Connected to MQTT server callback");
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

  // Send the discovery messages for the registered devices
  for (auto &device : connector->m_registered_devices) {
    device->sendDiscovery();
  }

  // Send status messages for all registered devices
  for (auto &device : connector->m_registered_devices) {
    device->sendStatus();
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
