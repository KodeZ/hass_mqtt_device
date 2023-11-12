/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

/**
 * This example shows how to create a simple dimmable light device. It fakes
 * changing the state of the light every 10 seconds. It will also respond to
 * control messages from the MQTT server, and respond with the current new
 * state. The device should be automatically discovered by Home Assistant.
 */

#include "hass_mqtt_device/core/mqtt_connector.h"
#include "hass_mqtt_device/devices/dimmable_light.h"
#include "hass_mqtt_device/logger/logger.hpp"
#include <fstream>
#include <iostream>
#include <memory>

bool _state = false;
double _brightness = 0.0;
bool _updated = true;

void controlCallback(bool state, double brightness)
{
    if(state != _state)
    {
        _state = state;
        _updated = true;
        LOG_INFO("State changed to {}", state);
    }
    else
    {
        LOG_INFO("State already set to {}", state);
    }

    if(brightness != _brightness)
    {
        _brightness = brightness;
        _updated = true;
        LOG_INFO("Brightness changed to {}", brightness);
    }
    else
    {
        LOG_INFO("Brightness already set to {}", brightness);
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

    // Create a light device with the name "light" and the unique id grabbed from
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
    unique_id += "_simple_dimmable_light";

    // Create the device
    auto light = std::make_shared<DimmableLightDevice>("simple_dimmable_light_example", controlCallback);
    light->init();

    auto connector = std::make_shared<MQTTConnector>(ip, port, username, password, unique_id);
    connector->registerDevice(light);
    connector->connect();

    // Run the device
    int loop_count = 0;
    while(1)
    {
        // Process messages from the MQTT server for 1 second
        connector->processMessages(1000);

        // Every 10 seconds, change the state of the light
        if(loop_count % 10 == 0)
        {
            _state = !_state;
            _brightness = 1.0 - _brightness;
            _updated = true;
        }
        loop_count++;

        // Every second, check if there is an update to the state of the light, and
        // if so, update the state
        if(_updated)
        {
            light->update(_state, _brightness);
            _updated = false;
        }
    }
}
