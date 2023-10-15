/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

/**
 * This example shows how to create a simple on/off light device by creating
 * one device with multiple functions attached to it.
 * It fakes changing the state of the light every 10 seconds. It will also
 * respond to control messages from the MQTT server, and respond with the
 * current new state. The device should be automatically discovered by Home
 * Assistant.
 */

#include "hass_mqtt_device/core/device_base.h"
#include "hass_mqtt_device/core/mqtt_connector.h"
#include "hass_mqtt_device/functions/on_off_light.h"
#include "hass_mqtt_device/logger/logger.hpp"
#include <fstream>
#include <iostream>
#include <memory>

// Default values
const std::string _function_name_prefix = "simple_on_off_light_";
const int _function_count = 5;
bool _state[_function_count];
bool _state_updated[_function_count];

void controlStateCallback(int device, bool state)
{
    if(state != _state[device])
    {
        _state[device] = state;
        _state_updated[device] = true;
        LOG_INFO("State for {} changed to {}", device, state);
    }
    else
    {
        LOG_INFO("State for {} already set to {}", device, state);
    }
}

// The main function. It receives the ip, port, username and password of the
// MQTT server as arguments
int main(int argc, char* argv[])
{
    bool debug = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--debug" || arg == "-d") {
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

    // Set state and updated to false
    for(int i = 0; i < _function_count; i++)
    {
        _state[i] = false;
        _state_updated[i] = false;
    }

    // Get the Unique ID from /etc/machine-id
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

    // Create the connector
    auto connector = std::make_shared<MQTTConnector>(ip, port, username, password);

    // Create the device
    auto light = std::make_shared<DeviceBase>("simple_on_off_light_multiple_functions", unique_id);

    for(int i = 0; i < _function_count; i++)
    {
        std::string function_name = _function_name_prefix + std::to_string(i);
        std::shared_ptr<OnOffLightFunction> on_off_light_func =
            std::make_shared<OnOffLightFunction>(function_name, [i](bool state) { controlStateCallback(i, state); });
        std::shared_ptr<FunctionBase> on_off_light_base_func = on_off_light_func;
        light->registerFunction(on_off_light_base_func);
    }

    connector->registerDevice(light);
    connector->connect();

    // Run the device
    int loop_count = 0;
    while(1)
    {
        LOG_DEBUG("Loop count: {}", loop_count);
        // Process messages from the MQTT server for 1 second
        connector->processMessages(1000);

        // Every 10 seconds, change the state of the light
        if(loop_count % 10 == 0)
        {
            for(int i = 0; i < _function_count; i++)
            {
                _state[i] = !_state[i];
                _state_updated[i] = true;
            }
        }
        loop_count++;

        // Every second, check if there is an update to the state of the light, and
        // if so, update the state
        for(int i = 0; i < _function_count; i++)
        {
            if(_state_updated[i])
            {
                LOG_INFO("Updating state for {}", i);

                std::string function_name = _function_name_prefix + std::to_string(i);

                std::shared_ptr<OnOffLightFunction> on_off_light =
                    std::dynamic_pointer_cast<OnOffLightFunction>(light->findFunction(function_name));
                if(on_off_light)
                {
                    on_off_light->update(_state[i]);
                    _state_updated[i] = false;
                }
                else
                {
                    LOG_ERROR("Could not find on_off_light function");
                }
            }
        }
    }
}
