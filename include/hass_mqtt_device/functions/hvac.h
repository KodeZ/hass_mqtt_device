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

    /**
     * @brief Get the temperature measured by the device
     *
     * @return The temperature measured by the device
     */
    [[nodiscard]] double getTemperature() const { return m_temperature; }

    /**
     * @brief Get the heating setpoint of the device
     *
     * @return The heating setpoint of the device
     */
    [[nodiscard]] double getHeatingSetpoint() const { return m_heating_setpoint; }

    /**
     * @brief Get the cooling setpoint of the device
     *
     * @return The cooling setpoint of the device
     */
    [[nodiscard]] double getCoolingSetpoint() const { return m_cooling_setpoint; }

    /**
     * @brief Get the humidity measured by the device
     *
     * @return The humidity measured by the device
     */
    [[nodiscard]] double getHumidity() const { return m_humidity; }

    /**
     * @brief Get the humidity setpoint of the device
     *
     * @return The humidity setpoint of the device
     */
    [[nodiscard]] double getHumiditySetpoint() const { return m_humidity_setpoint; }

    /**
     * @brief Get the fan mode of the device
     *
     * @return The fan mode of the device
     */
    [[nodiscard]] std::string getFanMode() const { return m_fan_mode; }

    /**
     * @brief Get the swing mode of the device
     *
     * @return The swing mode of the device
     */
    [[nodiscard]] std::string getSwingMode() const { return m_swing_mode; }

    /**
     * @brief Get the device mode of the device
     *
     * @return The device mode of the device
     */
    [[nodiscard]] std::string getDeviceMode() const { return m_device_mode; }

    /**
     * @brief Get the power state of the device
     *
     * @return The power state of the device
     */
    [[nodiscard]] bool getPowerState() const { return m_power; }

    /**
     * @brief Get the action state of the device
     *
     * @return The action state of the device
     */
    [[nodiscard]] HvacAction getAction() const { return m_action; }

    /**
     * @brief Get the preset mode of the device
     *
     * @return The preset mode of the device
     */
    [[nodiscard]] std::string getPresetMode() const { return m_preset_mode; }

    /**
     * @brief Get the supported features of the device
     *
     * @return The supported features of the device
     */
    [[nodiscard]] unsigned getSupportedFeatures() const { return m_supported_features; }

    /**
     * @brief Get the supported device modes of the device
     *
     * @return The supported device modes of the device
     */
    [[nodiscard]] std::vector<std::string> getDeviceModes() const { return m_device_modes; }

    /**
     * @brief Get the supported fan modes of the device
     *
     * @return The supported fan modes of the device
     */
    [[nodiscard]] std::vector<std::string> getFanModes() const { return m_fan_modes; }

    /**
     * @brief Get the supported swing modes of the device
     *
     * @return The supported swing modes of the device
     */
    [[nodiscard]] std::vector<std::string> getSwingModes() const { return m_swing_modes; }

    /**
     * @brief Get the supported preset modes of the device
     *
     * @return The supported preset modes of the device
     */
    [[nodiscard]] std::vector<std::string> getPresetModes() const { return m_preset_modes; }

    /**
     * @brief Get the last device mode of the device
     *
     * @return The last device mode of the device
     */
    [[nodiscard]] std::string getLastDeviceMode() const { return m_device_mode_last; }

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
