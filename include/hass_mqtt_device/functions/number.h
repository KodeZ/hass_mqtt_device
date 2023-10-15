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

/**
 * @brief Class for an on/off only light device
 *
 * Derived from function base
 */

class NumberFunction : public FunctionBase
{
public:
    /**
     * @brief Construct a new NumberFunction object
     *
     * @param parentDevice Reference to the parent device
     * @param functionName The name of the function
     */
    NumberFunction(const std::string& functionName,
                   std::function<void(double)> control_cb,
                   double max = 100,
                   double min = 0,
                   double step = 1);

    /**
     * @brief Destroy the NumberFunction object
     */
    ~NumberFunction() = default;

    /**
     * @brief Implement init function for this function
     */
    void init() override;

    /**
     * @brief Implements the subscribe topics function for this function
     *
     * @return The MQTT topic for this function
     */
    std::vector<std::string> getSubscribeTopics() const override;

    /**
     * @brief Implements the discovery topic function for this function
     *
     * @return The discovery topic for this function
     */
    std::string getDiscoveryTopic() const override;

    /**
     * @brief Implements the discovery payload function for this function
     *
     * @return The discovery payload for this function
     */
    json getDiscoveryJson() const override;

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
     * @brief Set the state of this function
     *
     * @param state The state to set
     */
    void update(double number);

    /**
     * @brief Get the brightness of this function
     *
     * @return The brightness of this function
     */
    bool getNumber() const
    {
        return m_number;
    };

private:
protected:
    double m_number;
    double m_max;
    double m_min;
    double m_step;
    std::function<void(double)> m_control_cb;
};
