/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

/**
 * This example shows how to create a simple number device. It fakes changing
 * the number every 10 seconds. It will also respond to control messages from
 * the MQTT server, and respond with the current new number. The device should
 * be automatically discovered by Home Assistant.
 */

#include "hass_mqtt_device/core/mqtt_connector.h"
#include "hass_mqtt_device/devices/number.h"
#include "hass_mqtt_device/logger/logger.hpp"
#include <fstream>
#include <iostream>
#include <memory>

double _number = 0.0;
bool _updated = true;

void controlCallback(double number)
{
    if(number != _number)
    {
        _number = number;
        _updated = true;
        LOG_INFO("number changed to {}", number);
    }
    else
    {
        LOG_INFO("number already set to {}", number);
    }
}

// The main function. It receives the ip, port, username and password of the
// MQTT server as arguments
int main(int argc, char* argv[])
{
    INIT_LOGGER_DEBUG();

    // Check and read the arguments
    if(argc != 5)
    {
        std::cout << "Usage: " << argv[0] << " <ip> <port> <username> <password>" << std::endl;
        return 1;
    }
    std::string ip = argv[1];
    int port = std::stoi(argv[2]);
    std::string username = argv[3];
    std::string password = argv[4];

    // Create a number device
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
    unique_id += "_simple_number_example";

    // Create the device
    auto number = std::make_shared<NumberDevice>("simple_number_example", unique_id, controlCallback);
    number->init();

    auto connector = std::make_shared<MQTTConnector>(ip, port, username, password);
    connector->registerDevice(number);
    connector->connect();

    // Run the device
    int loop_count = 0;
    while(1)
    {
        // Process messages from the MQTT server for 1 second
        connector->processMessages(1000);

        // Every 10 seconds, change the number of the number
        if(loop_count % 10 == 0)
        {
            _number = 100 - _number;
            _updated = true;
        }
        loop_count++;

        // Every second, check if there is an update to the number of the number,
        // and if so, update the number
        if(_updated)
        {
            number->update(_number);
            _updated = false;
        }
    }
}
