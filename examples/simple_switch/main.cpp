/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

/**
 * This example shows how to create a simple on/off switch device. It fakes
 * changing the state of the switch every 10 seconds. It will also respond to
 * control messages from the MQTT server, and respond with the current new
 * state. The device should be automatically discovered by Home Assistant.
 */

#include "hass_mqtt_device/core/mqtt_connector.h"
#include "hass_mqtt_device/devices/switch.h"
#include "hass_mqtt_device/logger/logger.hpp"
#include <fstream>
#include <iostream>
#include <memory>

bool _state = false;
bool _state_updated = true;

void controlStateCallback(bool state)
{
    if(state != _state)
    {
        _state = state;
        _state_updated = true;
        LOG_INFO("State changed to {}", state);
    }
    else
    {
        LOG_INFO("State already set to {}", state);
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

    // Create a switch device with the name "switch" and the unique id grabbed from
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
    unique_id += "_simple_switch";

    // Create the device
    auto sw = std::make_shared<SwitchDevice>("simple_switch_example", unique_id, controlStateCallback);
    sw->init();

    auto connector = std::make_shared<MQTTConnector>(ip, port, username, password);
    connector->registerDevice(sw);
    connector->connect();

    // Run the device
    int loop_count = 0;
    while(1)
    {
        // Process messages from the MQTT server for 1 second
        connector->processMessages(1000);

        // Every 10 seconds, change the state of the switch
        if(loop_count % 10 == 0)
        {
            _state = !_state;
            _state_updated = true;
        }
        loop_count++;

        // Every second, check if there is an update to the state of the switch, and
        // if so, update the state
        if(_state_updated)
        {
            sw->update(_state);
            _state_updated = false;
        }
    }
}
