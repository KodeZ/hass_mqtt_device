/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

/**
 * This example shows how to create a simple hvac device. The device should be automatically discovered by Home Assistant.
 */

#include "hass_mqtt_device/core/mqtt_connector.h"
#include "hass_mqtt_device/devices/hvac.h"
#include "hass_mqtt_device/functions/hvac.h"
#include "hass_mqtt_device/logger/logger.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

void controlStateCallback(std::shared_ptr<HvacFunction> function, HvacSupportedFeatures feature, std::string value)
{
    LOG_INFO("Control callback called. Feature: {}, value: {}", feature, value);
    if(feature == HvacSupportedFeatures::TEMPERATURE_CONTROL_HEATING)
    {
        function->updateHeatingSetpoint(std::stod(value));
    }
    else if(feature == HvacSupportedFeatures::TEMPERATURE_CONTROL_COOLING)
    {
        function->updateCoolingSetpoint(std::stod(value));
    }
    else if(feature == HvacSupportedFeatures::MODE_CONTROL)
    {
        function->updateDeviceMode(value);
    }
    else if(feature == HvacSupportedFeatures::FAN_MODE)
    {
        function->updateFanMode(value);
    }
    else if(feature == HvacSupportedFeatures::SWING_MODE)
    {
        function->updateSwingMode(value);
    }
    else if(feature == HvacSupportedFeatures::HUMIDITY_CONTROL)
    {
        function->updateHumiditySetpoint(std::stod(value));
    }
    else if(feature == HvacSupportedFeatures::POWER_CONTROL)
    {
        function->updatePowerState(value == "on");
    }
    else if(feature == HvacSupportedFeatures::PRESET_SUPPORT)
    {
        function->updatePresetMode(value);
    }
    else
    {
        LOG_ERROR("Unknown feature: {}", feature);
    }
}

// The main function. It receives the ip, port, username and password of the
// MQTT server as arguments
int main(int argc, char* argv[])
{
    bool debug = false;
    for(int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if(arg == "--debug" || arg == "-d")
        {
            debug = true;
            break;
        }
    }
    INIT_LOGGER(debug);

    // Check and read the arguments
    if(argc < 5)
    {
        std::cout << "Usage: " << argv[0] << " <ip> <port> <username> <password> [-d]" << std::endl;
        return 1;
    }
    std::string ip = argv[1];
    int port = std::stoi(argv[2]);
    std::string username = argv[3];
    std::string password = argv[4];

    // Create a hvac device with the name "hvac" and the unique id grabbed from
    // /etc/machine-id
    std::string unique_id;
    std::ifstream machine_id_file("/etc/machine-id");
    if(machine_id_file.good())
    {
        std::getline(machine_id_file, unique_id);
        machine_id_file.close();
    }
    else
    {
        std::cout << "Could not open /etc/machine-id" << std::endl;
        return 1;
    }
    unique_id += "_simple_hvac";

    // Create the device
    std::vector<std::string> modes = {"off", "heat", "cool", "auto", "dry", "fan_only"};
    std::vector<std::string> fan_modes = {"auto", "low", "medium", "high"};
    std::vector<std::string> swing_modes = {"off", "on"};
    std::vector<std::string> preset_modes = {"eco", "away"};

    auto sw = std::make_shared<HvacDevice>("simple_hvac_example", unique_id);

    sw->init([sw](HvacSupportedFeatures feature,
                  std::string value) { controlStateCallback(sw->getFunction(), feature, value); },
             HvacSupportedFeatures::TEMPERATURE | HvacSupportedFeatures::TEMPERATURE_CONTROL_HEATING |
                 HvacSupportedFeatures::TEMPERATURE_CONTROL_COOLING | HvacSupportedFeatures::MODE_CONTROL |
                 HvacSupportedFeatures::FAN_MODE | HvacSupportedFeatures::SWING_MODE |
                 HvacSupportedFeatures::HUMIDITY_CONTROL | HvacSupportedFeatures::HUMIDITY |
                 HvacSupportedFeatures::POWER_CONTROL | HvacSupportedFeatures::ACTION |
                 HvacSupportedFeatures::PRESET_SUPPORT
             ,
             modes,
             fan_modes,
             swing_modes,
             preset_modes);

    auto connector = std::make_shared<MQTTConnector>(ip, port, username, password);
    connector->registerDevice(sw);
    connector->connect();

    // Run the device
    int loop_count = 0;
    while(1)
    {
        // Process messages from the MQTT server for 1 second
        connector->processMessages(1000);

        // Every 11 seconds, change the state of the hvac
        if(loop_count % 11 == 0)
        {
            sw->getFunction()->updateTemperature(20.0 + (loop_count % 5));
            if(loop_count % 2 == 0)
            {
                sw->getFunction()->updateAction(HvacAction::HEATING);
            }
            else
            {
                sw->getFunction()->updateAction(HvacAction::COOLING);
            }
        }
        loop_count++;
    }
}
