/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#pragma once

#include "hass_mqtt_device/core/device_base.h"
#include "hass_mqtt_device/core/function_base.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Class for a hvac device
 *
 * Derived from function base
 */

enum HvacSupportedFeatures : unsigned
{
    TEMPERATURE = 0x0001,
    TEMPERATURE_CONTROL_HEATING = 0x0002,
    TEMPERATURE_CONTROL_COOLING = 0x0004,
    HUMIDITY = 0x0010,
    HUMIDITY_CONTROL = 0x0020,
    FAN_MODE = 0x0100,
    SWING_MODE = 0x0200,
    POWER_CONTROL = 0x1000, // On/Off control
    MODE_CONTROL = 0x2000, // Auto, Cool, Heat, Dry, Fan only type
    ACTION = 0x4000, // Reports what the device is currently doing, see HvacAction
    PRESET_SUPPORT = 0x8000
};

enum class HvacAction
{
    OFF,
    HEATING,
    COOLING,
    DRYING,
    IDLE,
    FAN
};

class HvacFunction : public FunctionBase
{
public:
    /**
     * @brief Construct a new HvacFunction object
     *
     * @param function_name The name of the function
     * @param supported_features The supported features of this hvac. This is a bitfield, so you or the features you want from the enum
     * @param control_cb The callback function for controlling the device
     */
    HvacFunction(const std::string& function_name,
                 std::function<void(HvacSupportedFeatures, std::string)> control_cb,
                 unsigned supported_features,
                 std::vector<std::string> device_modes = {},
                 std::vector<std::string> fan_modes = {},
                 std::vector<std::string> swing_modes = {},
                 std::vector<std::string> preset_modes = {} );

    /**
     * @brief Implement init function for this function
     */
    void init() override;

    /**
     * @brief Implements the subscribe topics function for this function
     *
     * @return The MQTT topic for this function
     */
    [[nodiscard]] std::vector<std::string> getSubscribeTopics() const override;

    /**
     * @brief Implements the discovery topic function for this function
     *
     * @return The discovery topic for this function
     */
    [[nodiscard]] std::string getDiscoveryTopic() const override;

    /**
     * @brief Implements the discovery payload function for this function
     *
     * @return The discovery payload for this function
     */
    [[nodiscard]] json getDiscoveryJson() const override;

    /**
     * @brief Implement process message function for this function
     *
     * @param topic The topic of the message
     * @param payload The payload of the message
     */
    void processMessage(const std::string& topic, const std::string& payload) override;

    /**
     * @brief Implement sending status for all values
     */
    void sendStatus() const override;

    /**
     * @brief Set the temperature measured by the device
     *
     * @param temperature The temperature measured by the device
     * @param send_status If true, send full status to the broker
     */
    void updateTemperature(double temperature, bool send_status = true);

    /**
     * @brief Set the heating setpoint of the device
     *
     * @param cooling_setpoint The heating setpoint of the device
     * @param send_status If true, send full status to the broker
     */
    void updateHeatingSetpoint(double heating_setpoint, bool send_status = true);

    /**
     * @brief Set the cooling setpoint of the device
     *
     * @param cooling_setpoint The cooling setpoint of the device
     * @param send_status If true, send full status to the broker
     */
    void updateCoolingSetpoint(double cooling_setpoint, bool send_status = true);

    /**
     * @brief Set the humidity measured by the device
     *
     * @param humidity The humidity measured by the device
     * @param send_status If true, send full status to the broker
     */
    void updateHumidity(double humidity, bool send_status = true);

    /**
     * @brief Set the humidity setpoint of the device
     *
     * @param humidity_setpoint The humidity setpoint of the device
     * @param send_status If true, send full status to the broker
     */
    void updateHumiditySetpoint(double humidity_setpoint, bool send_status = true);

    /**
     * @brief Set the fan mode of the device
     *
     * @param fan_mode The fan mode of the device
     * @param send_status If true, send full status to the broker
     */
    void updateFanMode(const std::string& fan_mode, bool send_status = true);

    /**
     * @brief Set the swing mode of the device
     *
     * @param swing_mode The swing mode of the device
     * @param send_status If true, send full status to the broker
     */
    void updateSwingMode(const std::string& swing_mode, bool send_status = true);

    /**
     * @brief Set the device mode of the device
     *
     * @param device_mode The device mode of the device
     * @param send_status If true, send full status to the broker
     */
    void updateDeviceMode(const std::string& device_mode, bool send_status = true);

    /**
     * @brief Set the power state of the device
     *
     * @param power The power state of the device
     * @param send_status If true, send full status to the broker
     */
    void updatePowerState(bool power, bool send_status = true);

    /**
     * @brief Set the action state of the device
     *
     * @param action The action state of the device
     * @param send_status If true, send full status to the broker
     */
    void updateAction(HvacAction action, bool send_status = true);

    /**
     * @brief Set the preset mode of the device
     *
     * @param preset_mode The preset mode of the device
     * @param send_status If true, send full status to the broker
     */
    void updatePresetMode(const std::string& preset_mode, bool send_status = true);

private:
protected:
    /**
     * @brief Implement sending status for one value
     *
     * @param feature The feature to send status for
     */
    void sendFunctionStatus(const HvacSupportedFeatures& feature) const;

    unsigned m_supported_features;
    std::function<void(HvacSupportedFeatures, std::string)> m_control_cb;
    std::vector<std::string> m_device_modes; // Auto, Cool, Heat, Dry, Fan only type modes
    std::vector<std::string> m_fan_modes;
    std::vector<std::string> m_swing_modes;
    std::vector<std::string> m_preset_modes;
    bool m_power;
    double m_temperature;
    double m_cooling_setpoint;
    double m_heating_setpoint;
    double m_humidity;
    double m_humidity_setpoint;
    HvacAction m_action;
    std::string m_device_mode; // Auto, Cool, Heat, Dry, Fan only type modes
    std::string m_device_mode_last; // Used if turned off via power control, to remember the last mode
    std::string m_fan_mode;
    std::string m_swing_mode;
    std::string m_preset_mode;
};
