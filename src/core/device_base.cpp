/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

// Include the corresponding header file
#include "hass_mqtt_device/core/device_base.h"
#include "hass_mqtt_device/core/function_base.h"

// Include any other necessary headers
#include "hass_mqtt_device/logger/logger.hpp" // For logging

DeviceBase::DeviceBase(const std::string &device_name,
                       const std::string &unique_id)
    : m_device_name(device_name), m_unique_id(unique_id) {}

std::string DeviceBase::getId() const { return m_unique_id; }

std::string DeviceBase::getName() const { return m_device_name; }
std::string DeviceBase::getFullId() const {
  return m_unique_id + "_" + m_device_name;
}

std::vector<std::string> DeviceBase::getSubscribeTopics() const {
  // Loop through all functions and get their topics
  std::vector<std::string> topics;
  for (auto &function : m_functions) {
    auto functionTopics = function->getSubscribeTopics();
    topics.insert(topics.end(), functionTopics.begin(), functionTopics.end());
  }
  // Make sure there are no duplicates, throw an error if there are
  std::sort(topics.begin(), topics.end());
  auto last = std::unique(topics.begin(), topics.end());
  if (last != topics.end()) {
    LOG_ERROR("Duplicate topics found for device {}", m_device_name);
    throw std::runtime_error("Duplicate topics found for device");
  }
  return topics;
}

void DeviceBase::registerFunction(std::shared_ptr<FunctionBase> function) {
  // Check if the function wit the same discovery topic already exists
  for (auto &existingFunction : m_functions) {
    if (existingFunction->getName() == function->getName()) {
      LOG_ERROR("Function with discovery topic {} already exists",
                function->getDiscoveryTopic());
      throw std::runtime_error("Function with discovery topic already exists");
    }
  }
  function->setParentDevice(shared_from_this());
  m_functions.push_back(function);
}

std::shared_ptr<FunctionBase>
DeviceBase::findFunction(const std::string &name) const {
  // Loop through all functions and check if the name matches
  for (auto &function : m_functions) {
    if (function->getName() == name) {
      return function;
    }
  }
  // If no function was found, return an empty pointer
  return std::shared_ptr<FunctionBase>();
}

void DeviceBase::sendDiscovery() {
  // Loop through all functions and gather their discovery parts
  std::map<std::string, json> discoveryParts;
  for (auto &function : m_functions) {
    auto discoveryTopic = function->getDiscoveryTopic();
    auto discoveryJson = function->getDiscoveryJson();
    // Check if the discovery topic already exists, throw an error if it does
    if (discoveryParts.find(discoveryTopic) != discoveryParts.end()) {
      LOG_ERROR("Duplicate discovery topic {} found for device {}",
                discoveryTopic, m_device_name);
      throw std::runtime_error("Duplicate discovery topic found for device");
    }

    discoveryJson["schema"] = "json";
    discoveryJson["availability_topic"] = "home/" + getId() + "/availability";
    discoveryJson["availability_template"] = "{{ value_json.availability }}";

    // Add the device info to the discovery json
    discoveryJson["device"] = {{"name", m_device_name},
                               {"identifiers", {m_unique_id}},
                               {"manufacturer", "Homebrew"},
                               {"model", "hass_mqtt_device"},
                               {"sw_version", "0.1.0"}};

    discoveryParts[discoveryTopic] = discoveryJson;
  }
  // Now to send the discovery messages
  for (auto &discoveryPart : discoveryParts) {
    LOG_DEBUG("Sending discovery message to topic: {}", discoveryPart.first);
    LOG_DEBUG("Discovery message: {}", discoveryPart.second.dump());
    try {
      publishMessage(discoveryPart.first, discoveryPart.second);
    } catch (const std::exception &e) {
      LOG_ERROR("Failed to send discovery message for device {}-{}: {}",
                getName(), getId(), e.what());
      throw e;
    }
  }
}

void DeviceBase::sendLWT() {
  // Create the will message
  json payload;
  payload["availability"] = "offline";
  // Publish the will message
  // Check if the connector is still alive
  if (auto connector = m_connector.lock()) {
    // Publish the message
    connector->publishLWT("home/" + getId() + "/availability", payload);
  } else {
    LOG_ERROR(
        "Failed to publish MQTT message: MQTTConnector is no longer alive");
    throw std::runtime_error(
        "Failed to publish MQTT message: MQTTConnector is no longer alive");
  }
}

void DeviceBase::processMessage(const std::string &topic,
                                const std::string &payload) {
  LOG_DEBUG("Processing message for device {} with topic {}", getName(), topic);
  // Loop through all functions and check if the topic matches
  for (auto &function : m_functions) {
    // Check if the topic starts with the function's topic
    if (topic.find(function->getName()) != std::string::npos) {
      // Call the function's onMessage method
      function->processMessage(topic, payload);
    }
  }
}

void DeviceBase::publishMessage(const std::string &topic, const json &payload) {
  // Check if the connector is still alive
  if (auto connector = m_connector.lock()) {
    // Publish the message
    connector->publishMessage(topic, payload);
  } else {
    LOG_ERROR(
        "Failed to publish MQTT message: MQTTConnector is no longer alive");
    throw std::runtime_error(
        "Failed to publish MQTT message: MQTTConnector is no longer alive");
  }
}

void DeviceBase::sendStatus() {
  // Loop through all functions and call their sendStatus method
  for (auto &function : m_functions) {
    function->sendStatus();
  }
  // Send online message
  json payload;
  payload["availability"] = "online";
  publishMessage("home/" + getId() + "/availability", payload);
}
