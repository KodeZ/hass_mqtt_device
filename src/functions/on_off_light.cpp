/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

// Include the corresponding header file
#include "hass_mqtt_device/functions/on_off_light.h"
#include "hass_mqtt_device/core/device_base.h"

// Include any other necessary headers
#include "hass_mqtt_device/logger/logger.hpp" // For logging

OnOffLightFunction::OnOffLightFunction(
    const std::string &functionName, std::function<void(bool)> control_state_cb)
    : FunctionBase(functionName), m_control_state_cb(control_state_cb) {}

void OnOffLightFunction::init() {
  LOG_DEBUG("Initializing on/off light function {}", getName());
  auto parent = m_parentDevice.lock();
  if (!parent) {
    LOG_ERROR("Parent device is no longer available.");
    return;
  }

  auto topic = "home/" + parent->getFullId() + "/light/" + getName() + "/set";
  m_sub_topics[topic] = [this](const std::string &payload) {
    try {
      json payloadJson = json::parse(payload);
      controlState(payloadJson);
    } catch (const json::exception &e) {
      LOG_ERROR("JSON error in payload for on/off light: {}. Error: {}",
                payload, e.what());
    }
  };
}

std::vector<std::string> OnOffLightFunction::getSubscribeTopics() const {
  // Create a vector of the topics
  std::vector<std::string> topics;
  for (auto &topic : m_sub_topics) {
    topics.push_back(topic.first);
  }
  return topics;
}

std::string OnOffLightFunction::getDiscoveryTopic() const {
  auto parent = m_parentDevice.lock();
  if (!parent) {
    LOG_ERROR("Parent device is not available.");
    return "";
  }
  return "homeassistant/light/" + parent->getFullId() + "/" + getName() +
         "/config";
}

json OnOffLightFunction::getDiscoveryJson() const {
  auto parent = m_parentDevice.lock();
  json discoveryJson;
  discoveryJson["name"] = parent->getName() + " " + getName();
  discoveryJson["unique_id"] =
      parent->getFullId() + "_" + getName(); // We use the parent id to enable HA to merge the
                           // functions into one device
  discoveryJson["command_topic"] =
      "home/" + parent->getFullId() + "/light/" + getName() + "/set";
  discoveryJson["state_topic"] =
      "home/" + parent->getFullId() + "/light/" + getName() + "/state";
  discoveryJson["schema"] = "json";
  discoveryJson["state_value_template"] = "{{ value_json.state }}";
  discoveryJson["command_on_template"] = "{\"state\": \"ON\"}";
  discoveryJson["command_off_template"] = "{\"state\": \"OFF\"}";

  return discoveryJson;
}

void OnOffLightFunction::processMessage(const std::string &topic,
                                        const std::string &payload) {
  // Check if the topic is in the map
  LOG_DEBUG("Processing message for on/off light function {} with topic {}",
            getName(), topic);
  if (m_sub_topics.find(topic) != m_sub_topics.end()) {
    // Call the function
    m_sub_topics[topic](payload);
  }
}

void OnOffLightFunction::sendStatus() const {
  auto parent = m_parentDevice.lock();
  if (!parent) {
    LOG_ERROR("Parent device is no longer available.");
    return;
  }

  json payload;
  payload["state"] = m_state ? "ON" : "OFF";
  parent->publishMessage("home/" + parent->getFullId() + "/light/" + getName() +
                             "/state",
                         payload);
}

void OnOffLightFunction::controlState(json state) {
  if (state["state"] == "ON") {
    m_control_state_cb(true);
  } else if (state["state"] == "OFF") {
    m_control_state_cb(false);
  } else {
    LOG_ERROR("Unexpected state value in payload for on/off light: {}",
              state.dump());
  }
}

void OnOffLightFunction::setState(bool state) {
  m_state = state;
  sendStatus();
}
