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
#include "hass_mqtt_device/logger/logger.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

void controlStateCallback(HvacSupportedFeatures feature, std::string value)
{
    LOG_INFO("Control callback called. Feature: {}, value: {}", feature, value);
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
    std::vector<std::string> preset_modes = {"none", "eco", "away"};

    auto sw = std::make_shared<HvacDevice>("simple_hvac_example", unique_id);

    sw->init(controlStateCallback,
             HvacSupportedFeatures::TEMPERATURE | HvacSupportedFeatures::TEMPERATURE_CONTROL_HEATING |
                 HvacSupportedFeatures::TEMPERATURE_CONTROL_COOLING | HvacSupportedFeatures::MODE_CONTROL |
                 HvacSupportedFeatures::FAN_MODE | HvacSupportedFeatures::SWING_MODE |
                 HvacSupportedFeatures::HUMIDITY_CONTROL | HvacSupportedFeatures::HUMIDITY |
                 HvacSupportedFeatures::POWER_CONTROL | HvacSupportedFeatures::ACTION |
                 HvacSupportedFeatures::PRESET_SUPPORT,
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

        // Every 10 seconds, change the state of the hvac
        if(loop_count % 10 == 0)
        {
        }
        loop_count++;
    }
}
