/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

// Include the corresponding header file
#include <utility>

#include "hass_mqtt_device/core/device_base.h"
#include "hass_mqtt_device/functions/hvac.h"

// Include any other necessary headers
#include "hass_mqtt_device/core/mqtt_connector.h"
#include "hass_mqtt_device/logger/logger.hpp" // For logging

HvacFunction::HvacFunction(const std::string& function_name,
                           std::function<void(HvacSupportedFeatures, std::string)> control_cb,
                           unsigned supported_features,
                           std::vector<std::string> device_modes,
                           std::vector<std::string> fan_modes,
                           std::vector<std::string> swing_modes,
                           std::vector<std::string> preset_modes)
    : FunctionBase(function_name)
    , m_control_cb(std::move(control_cb))
    , m_supported_features(supported_features)
    , m_device_modes(std::move(device_modes))
    , m_fan_modes(std::move(fan_modes))
    , m_swing_modes(std::move(swing_modes))
    , m_preset_modes(std::move(preset_modes))
    , m_power(false)
    , m_temperature(0.0)
    , m_heating_setpoint(18.0)
    , m_cooling_setpoint(25.0)
    , m_humidity_setpoint(60.0)
    , m_humidity(0.0)
    , m_fan_mode("auto")
    , m_swing_mode("off")
    , m_device_mode("off")
    , m_action(HvacAction::OFF)
    , m_preset_mode("none")
{
}

void HvacFunction::init()
{
    LOG_DEBUG("Initializing hvac function {}", getName());
}

std::vector<std::string> HvacFunction::getSubscribeTopics() const
{
    // Create a vector of the topics
    std::vector<std::string> topics;

    // Look through the supported features and add the corresponding topics
    if((m_supported_features & HvacSupportedFeatures::TEMPERATURE_CONTROL_HEATING) != 0U)
    {
        topics.push_back(getBaseTopic() + "heating_temperature/set");
    }
    if((m_supported_features & HvacSupportedFeatures::TEMPERATURE_CONTROL_COOLING) != 0U)
    {
        topics.push_back(getBaseTopic() + "cooling_temperature/set");
    }
    if((m_supported_features & HvacSupportedFeatures::HUMIDITY_CONTROL) != 0U)
    {
        topics.push_back(getBaseTopic() + "humidity/set");
    }
    if((m_supported_features & HvacSupportedFeatures::FAN_MODE) != 0U)
    {
        topics.push_back(getBaseTopic() + "fan_mode/set");
    }
    if((m_supported_features & HvacSupportedFeatures::SWING_MODE) != 0U)
    {
        topics.push_back(getBaseTopic() + "swing_mode/set");
    }
    if((m_supported_features & HvacSupportedFeatures::POWER_CONTROL) != 0U)
    {
        topics.push_back(getBaseTopic() + "set");
    }
    if((m_supported_features & HvacSupportedFeatures::MODE_CONTROL) != 0U)
    {
        topics.push_back(getBaseTopic() + "mode/set");
    }
    if((m_supported_features & HvacSupportedFeatures::PRESET_SUPPORT) != 0U)
    {
        topics.push_back(getBaseTopic() + "preset_mode/set");
    }

    return topics;
}

std::string HvacFunction::getDiscoveryTopic() const
{
    auto parent = m_parent_device.lock();
    if(!parent)
    {
        LOG_ERROR("Parent device is not available.");
        return "";
    }
    return "homeassistant/climate/" + parent->getFullId() + "/" + getCleanName() + "/config";
}

json HvacFunction::getDiscoveryJson() const
{
    auto parent = m_parent_device.lock();
    json discoveryJson;
    discoveryJson["name"] = getName();
    discoveryJson["unique_id"] = getId();
    // Adding enabled features
    if((m_supported_features & HvacSupportedFeatures::TEMPERATURE) != 0U)
    {
        discoveryJson["current_temperature_topic"] = getBaseTopic() + "temperature/measured";
        discoveryJson["current_temperature_template"] = "{{ value_json.temperature }}";
    }
    if((m_supported_features & HvacSupportedFeatures::TEMPERATURE_CONTROL_HEATING) != 0U)
    {
        discoveryJson["temperature_low_command_topic"] = getBaseTopic() + "heating_temperature/set";
        discoveryJson["temperature_low_command_template"] = R"({"value": "{{ value }}" })";
        discoveryJson["temperature_low_state_topic"] = getBaseTopic() + "heating_temperature/state";
        discoveryJson["temperature_low_state_template"] = "{{ value_json.value }}";
    }
    if((m_supported_features & HvacSupportedFeatures::TEMPERATURE_CONTROL_COOLING) != 0U)
    {
        discoveryJson["temperature_high_command_topic"] = getBaseTopic() + "cooling_temperature/set";
        discoveryJson["temperature_high_command_template"] = R"({"value": "{{ value }}" })";
        discoveryJson["temperature_high_state_topic"] = getBaseTopic() + "cooling_temperature/state";
        discoveryJson["temperature_high_state_template"] = "{{ value_json.value }}";
    }
    if((m_supported_features & HvacSupportedFeatures::HUMIDITY) != 0U)
    {
        discoveryJson["current_humidity_topic"] = getBaseTopic() + "humidity/measured";
        discoveryJson["current_humidity_template"] = "{{ value_json.humidity }}";
    }
    if((m_supported_features & HvacSupportedFeatures::HUMIDITY_CONTROL) != 0U) // Target humidity
    {
        discoveryJson["target_humidity_command_topic"] = getBaseTopic() + "humidity/set";
        discoveryJson["target_humidity_command_template"] = R"({"value": "{{ value }}" })";
        discoveryJson["target_humidity_state_topic"] = getBaseTopic() + "humidity/state";
        discoveryJson["target_humidity_state_template"] = "{{ value_json.value }}";
    }
    if((m_supported_features & HvacSupportedFeatures::FAN_MODE) != 0U)
    {
        discoveryJson["fan_mode_command_topic"] = getBaseTopic() + "fan_mode/set";
        discoveryJson["fan_mode_command_template"] = R"({"value": "{{ value }}" })";
        discoveryJson["fan_mode_state_topic"] = getBaseTopic() + "fan_mode/state";
        discoveryJson["fan_mode_state_template"] = "{{ value_json.value }}";
        discoveryJson["fan_modes"] = m_fan_modes;
    }
    if((m_supported_features & HvacSupportedFeatures::SWING_MODE) != 0U)
    {
        discoveryJson["swing_mode_command_topic"] = getBaseTopic() + "swing_mode/set";
        discoveryJson["swing_mode_command_template"] = R"({"value": "{{ value }}" })";
        discoveryJson["swing_mode_state_topic"] = getBaseTopic() + "swing_mode/state";
        discoveryJson["swing_mode_state_template"] = "{{ value_json.value }}";
        discoveryJson["swing_modes"] = m_swing_modes;
    }
    if((m_supported_features & HvacSupportedFeatures::POWER_CONTROL) != 0U)
    {
        discoveryJson["power_command_topic"] = getBaseTopic() + "set";
        discoveryJson["power_command_template"] = R"({"value": "{{ value }}" })";
        discoveryJson["payload_on"] = "on";
        discoveryJson["payload_off"] = "off";
    }
    if((m_supported_features & HvacSupportedFeatures::MODE_CONTROL) != 0U)
    {
        discoveryJson["mode_command_topic"] = getBaseTopic() + "mode/set";
        discoveryJson["mode_command_template"] = R"({"value": "{{ value }}" })";
        discoveryJson["mode_state_topic"] = getBaseTopic() + "mode/state";
        discoveryJson["mode_state_template"] = "{{ value_json.value }}";
        discoveryJson["modes"] = m_device_modes;
    }
    if((m_supported_features & HvacSupportedFeatures::ACTION) != 0U)
    {
        discoveryJson["action_topic"] = getBaseTopic() + "action/state";
        discoveryJson["action_template"] = "{{ value_json.action }}";
    }
    if((m_supported_features & HvacSupportedFeatures::PRESET_SUPPORT) != 0U)
    {
        discoveryJson["preset_mode_command_topic"] = getBaseTopic() + "preset_mode/set";
        discoveryJson["preset_mode_command_template"] = R"({"value": "{{ value }}" })";
        discoveryJson["preset_mode_state_topic"] = getBaseTopic() + "preset_mode/state";
        discoveryJson["preset_mode_value_template"] = "{{ value_json.value }}";
        discoveryJson["preset_modes"] = m_preset_modes;
    }

    return discoveryJson;
}

void HvacFunction::processMessage(const std::string& topic, const std::string& payload)
{
    LOG_DEBUG("Processing message for hvac function {} with topic {} and payload {}", getName(), topic, payload);

    // Check if the topic is really for us
    if(topic.find(getBaseTopic()) != 0U)
    {
        LOG_ERROR("Topic {} is not for this function", topic);
        return;
    }

    // Decode the payload
    json payloadJson;
    try
    {
        payloadJson = json::parse(payload);
    }
    catch(const json::exception& e)
    {
        LOG_ERROR("JSON error in payload: {}. Error: {}", payload, e.what());
        return;
    }

    auto value = payloadJson["value"].get<std::string>();

    // Handle the sub topics
    if((m_supported_features & HvacSupportedFeatures::TEMPERATURE_CONTROL_HEATING) != 0U)
    {
        if(topic == getBaseTopic() + "heating_temperature/set")
        {
            m_control_cb(HvacSupportedFeatures::TEMPERATURE_CONTROL_HEATING, value);
        }
    }
    if((m_supported_features & HvacSupportedFeatures::TEMPERATURE_CONTROL_COOLING) != 0U)
    {
        if(topic == getBaseTopic() + "cooling_temperature/set")
        {
            m_control_cb(HvacSupportedFeatures::TEMPERATURE_CONTROL_COOLING, value);
        }
    }
    if((m_supported_features & HvacSupportedFeatures::HUMIDITY_CONTROL) != 0U)
    {
        if(topic == getBaseTopic() + "humidity/set")
        {
            m_control_cb(HvacSupportedFeatures::HUMIDITY_CONTROL, value);
        }
    }
    if((m_supported_features & HvacSupportedFeatures::FAN_MODE) != 0U)
    {
        if(topic == getBaseTopic() + "fan_mode/set")
        {
            m_control_cb(HvacSupportedFeatures::FAN_MODE, value);
        }
    }
    if((m_supported_features & HvacSupportedFeatures::SWING_MODE) != 0U)
    {
        if(topic == getBaseTopic() + "swing_mode/set")
        {
            m_control_cb(HvacSupportedFeatures::SWING_MODE, value);
        }
    }
    if((m_supported_features & HvacSupportedFeatures::POWER_CONTROL) != 0U)
    {
        if(topic == getBaseTopic() + "set")
        {
            m_control_cb(HvacSupportedFeatures::POWER_CONTROL, value);
        }
    }
    if((m_supported_features & HvacSupportedFeatures::MODE_CONTROL) != 0U)
    {
        if(topic == getBaseTopic() + "mode/set")
        {
            m_control_cb(HvacSupportedFeatures::MODE_CONTROL, value);
        }
    }
    if((m_supported_features & HvacSupportedFeatures::PRESET_SUPPORT) != 0U)
    {
        if(topic == getBaseTopic() + "preset_mode/set")
        {
            m_control_cb(HvacSupportedFeatures::PRESET_SUPPORT, value);
        }
    }
}

void HvacFunction::sendStatus() const
{
    sendFunctionStatus(HvacSupportedFeatures::TEMPERATURE);
    sendFunctionStatus(HvacSupportedFeatures::TEMPERATURE_CONTROL_HEATING);
    sendFunctionStatus(HvacSupportedFeatures::TEMPERATURE_CONTROL_COOLING);
    sendFunctionStatus(HvacSupportedFeatures::HUMIDITY);
    sendFunctionStatus(HvacSupportedFeatures::HUMIDITY_CONTROL);
    sendFunctionStatus(HvacSupportedFeatures::FAN_MODE);
    sendFunctionStatus(HvacSupportedFeatures::SWING_MODE);
    sendFunctionStatus(HvacSupportedFeatures::MODE_CONTROL);
    sendFunctionStatus(HvacSupportedFeatures::ACTION);
    sendFunctionStatus(HvacSupportedFeatures::PRESET_SUPPORT);
}

void HvacFunction::sendFunctionStatus(const HvacSupportedFeatures& feature) const
{
    auto parent = m_parent_device.lock();
    if(!parent)
    {
        return;
    }

    if(feature == HvacSupportedFeatures::TEMPERATURE && (m_supported_features & HvacSupportedFeatures::TEMPERATURE) != 0U)
    {
        json payload;
        payload["temperature"] = m_temperature;
        parent->publishMessage(getBaseTopic() + "temperature/measured", payload);
        return;
    }
    if(feature == HvacSupportedFeatures::TEMPERATURE_CONTROL_HEATING &&
       (m_supported_features & HvacSupportedFeatures::TEMPERATURE_CONTROL_HEATING) != 0U)
    {
        json payload;
        payload["value"] = m_heating_setpoint;
        parent->publishMessage(getBaseTopic() + "heating_temperature/state", payload);
        return;
    }
    if(feature == HvacSupportedFeatures::TEMPERATURE_CONTROL_COOLING &&
       (m_supported_features & HvacSupportedFeatures::TEMPERATURE_CONTROL_COOLING) != 0U)
    {
        json payload;
        payload["value"] = m_cooling_setpoint;
        parent->publishMessage(getBaseTopic() + "cooling_temperature/state", payload);
        return;
    }
    if(feature == HvacSupportedFeatures::HUMIDITY && (m_supported_features & HvacSupportedFeatures::HUMIDITY) != 0U)
    {
        json payload;
        payload["humidity"] = m_humidity;
        parent->publishMessage(getBaseTopic() + "humidity/measured", payload);
        return;
    }
    if(feature == HvacSupportedFeatures::HUMIDITY_CONTROL &&
       (m_supported_features & HvacSupportedFeatures::HUMIDITY_CONTROL) != 0U)
    {
        json payload;
        payload["value"] = m_humidity_setpoint;
        parent->publishMessage(getBaseTopic() + "humidity/state", payload);
        return;
    }
    if(feature == HvacSupportedFeatures::FAN_MODE && (m_supported_features & HvacSupportedFeatures::FAN_MODE) != 0U)
    {
        json payload;
        payload["value"] = m_fan_mode;
        parent->publishMessage(getBaseTopic() + "fan_mode/state", payload);
        return;
    }
    if(feature == HvacSupportedFeatures::SWING_MODE && (m_supported_features & HvacSupportedFeatures::SWING_MODE) != 0U)
    {
        json payload;
        payload["value"] = m_swing_mode;
        parent->publishMessage(getBaseTopic() + "swing_mode/state", payload);
        return;
    }
    if(feature == HvacSupportedFeatures::MODE_CONTROL &&
       (m_supported_features & HvacSupportedFeatures::MODE_CONTROL) != 0U)
    {
        json payload;
        payload["value"] = m_device_mode;
        parent->publishMessage(getBaseTopic() + "mode/state", payload);
        return;
    }
    if(feature == HvacSupportedFeatures::ACTION && (m_supported_features & HvacSupportedFeatures::ACTION) != 0U)
    {
        json payload;
        switch(m_action)
        {
            case HvacAction::OFF:
                payload["action"] = "off";
                break;
            case HvacAction::HEATING:
                payload["action"] = "heating";
                break;
            case HvacAction::COOLING:
                payload["action"] = "cooling";
                break;
            case HvacAction::DRYING:
                payload["action"] = "drying";
                break;
            case HvacAction::IDLE:
                payload["action"] = "idle";
                break;
            case HvacAction::FAN:
                payload["action"] = "fan";
                break;
            default:
                payload["action"] = nullptr;
                break;
        }
        parent->publishMessage(getBaseTopic() + "action/state", payload);
        return;
    }
    if(feature == HvacSupportedFeatures::PRESET_SUPPORT &&
       (m_supported_features & HvacSupportedFeatures::PRESET_SUPPORT) != 0U)
    {
        json payload;
        payload["value"] = m_preset_mode;
        parent->publishMessage(getBaseTopic() + "preset_mode/state", payload);
        return;
    }
    LOG_DEBUG("Feature {} is not supported for this hvac function", feature);
}

void HvacFunction::updateTemperature(double temperature, bool send_status)
{
    if((m_supported_features & HvacSupportedFeatures::TEMPERATURE) == 0U)
    {
        LOG_ERROR("Temperature is not supported for this hvac function.");
        return;
    }
    m_temperature = temperature;
    if(send_status)
    {
        sendFunctionStatus(HvacSupportedFeatures::TEMPERATURE);
    }
}

void HvacFunction::updateHeatingSetpoint(double heating_setpoint, bool send_status)
{
    if((m_supported_features & HvacSupportedFeatures::TEMPERATURE_CONTROL_HEATING) == 0U)
    {
        LOG_ERROR("Heating setpoint is not supported for this hvac function.");
        return;
    }
    m_heating_setpoint = heating_setpoint;
    if(send_status)
    {
        sendFunctionStatus(HvacSupportedFeatures::TEMPERATURE_CONTROL_HEATING);
    }
}

void HvacFunction::updateCoolingSetpoint(double cooling_setpoint, bool send_status)
{
    if((m_supported_features & HvacSupportedFeatures::TEMPERATURE_CONTROL_COOLING) == 0U)
    {
        LOG_ERROR("Cooling setpoint is not supported for this hvac function.");
        return;
    }
    m_cooling_setpoint = cooling_setpoint;
    if(send_status)
    {
        sendFunctionStatus(HvacSupportedFeatures::TEMPERATURE_CONTROL_COOLING);
    }
}

void HvacFunction::updateHumidity(double humidity, bool send_status)
{
    if((m_supported_features & HvacSupportedFeatures::HUMIDITY) == 0U)
    {
        LOG_ERROR("Humidity is not supported for this hvac function.");
        return;
    }
    m_humidity = humidity;
    if(send_status)
    {
        sendFunctionStatus(HvacSupportedFeatures::HUMIDITY);
    }
}

void HvacFunction::updateHumiditySetpoint(double humidity_setpoint, bool send_status)
{
    if((m_supported_features & HvacSupportedFeatures::HUMIDITY_CONTROL) == 0U)
    {
        LOG_ERROR("Humidity setpoint is not supported for this hvac function.");
        return;
    }
    m_humidity_setpoint = humidity_setpoint;
    if(send_status)
    {
        sendFunctionStatus(HvacSupportedFeatures::HUMIDITY_CONTROL);
    }
}

void HvacFunction::updateFanMode(const std::string& fan_mode, bool send_status)
{
    if((m_supported_features & HvacSupportedFeatures::FAN_MODE) == 0U)
    {
        LOG_ERROR("Fan mode is not supported for this hvac function.");
        return;
    }
    m_fan_mode = fan_mode;
    if(send_status)
    {
        sendFunctionStatus(HvacSupportedFeatures::FAN_MODE);
    }
}

void HvacFunction::updateSwingMode(const std::string& swing_mode, bool send_status)
{
    if((m_supported_features & HvacSupportedFeatures::SWING_MODE) == 0U)
    {
        LOG_ERROR("Swing mode is not supported for this hvac function.");
        return;
    }
    m_swing_mode = swing_mode;
    if(send_status)
    {
        sendFunctionStatus(HvacSupportedFeatures::SWING_MODE);
    }
}

void HvacFunction::updatePowerState(bool power, bool send_status)
{
    if((m_supported_features & HvacSupportedFeatures::POWER_CONTROL) == 0U)
    {
        LOG_ERROR("Device mode is not supported for this hvac function.");
        return;
    }
    m_power = power;
    if(!m_power)
    {
        m_device_mode_last = m_device_mode;
    }
    m_device_mode = m_power ? m_device_mode_last : "off";
    if(send_status)
    {
        sendFunctionStatus(HvacSupportedFeatures::MODE_CONTROL);
    }
}

void HvacFunction::updateDeviceMode(const std::string& device_mode, bool send_status)
{
    if((m_supported_features & HvacSupportedFeatures::MODE_CONTROL) == 0U)
    {
        LOG_ERROR("Device mode is not supported for this hvac function.");
        return;
    }
    m_device_mode = device_mode;
    m_power = m_device_mode != "off";
    if(send_status)
    {
        sendFunctionStatus(HvacSupportedFeatures::MODE_CONTROL);
    }
}

void HvacFunction::updateAction(HvacAction action, bool send_status)
{
    if((m_supported_features & HvacSupportedFeatures::ACTION) == 0U)
    {
        LOG_ERROR("Action is not supported for this hvac function.");
        return;
    }
    m_action = action;
    if(send_status)
    {
        sendFunctionStatus(HvacSupportedFeatures::ACTION);
    }
}

void HvacFunction::updatePresetMode(const std::string& preset_mode, bool send_status)
{
    if((m_supported_features & HvacSupportedFeatures::PRESET_SUPPORT) == 0U)
    {
        LOG_ERROR("Preset mode is not supported for this hvac function.");
        return;
    }
    m_preset_mode = preset_mode;
    if(send_status)
    {
        sendFunctionStatus(HvacSupportedFeatures::PRESET_SUPPORT);
    }
}
