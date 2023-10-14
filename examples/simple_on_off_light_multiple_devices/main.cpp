/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

/**
 * This example shows how to create a simple on/off light device by creating
 * multiple devices with one function each.
 * I do not think this is the best way to do it, but it is possible. The
 * "correct" way would be to create one device with multiple functions attached
 * to it. See examples/simple_on_off_light_multiple_functions for an example of
 * that.
 * This example fakes changing the state of the light every 10 seconds. It will
 * also respond to control messages from the MQTT server, and respond with the
 * current new state. The device should be automatically discovered by Home
 * Assistant.
 */

#include "hass_mqtt_device/core/mqtt_connector.h"
#include "hass_mqtt_device/devices/on_off_light.h"
#include "hass_mqtt_device/logger/logger.hpp"
#include <fstream>
#include <iostream>
#include <memory>

// Default values
const std::string _device_name_prefix =
    "simple_on_off_light_multiple_devices_example_";
const int _device_count = 5;
bool _state[_device_count];
bool _state_updated[_device_count];

void controlStateCallback(int device, bool state) {
  if (state != _state[device]) {
    _state[device] = state;
    _state_updated[device] = true;
    LOG_INFO("State for {} changed to {}", device, state);
  } else {
    LOG_INFO("State for {} already set to {}", device, state);
  }
}

// The main function. It receives the ip, port, username and password of the
// MQTT server as arguments
int main(int argc, char *argv[]) {
  INIT_LOGGER_DEBUG();

  // Check and read the arguments
  if (argc != 5) {
    std::cout << "Usage: " << argv[0] << " <ip> <port> <username> <password>"
              << std::endl;
    return 1;
  }
  std::string ip = argv[1];
  int port = std::stoi(argv[2]);
  std::string username = argv[3];
  std::string password = argv[4];

  // Set state and updated to false
  for (int i = 0; i < _device_count; i++) {
    _state[i] = false;
    _state_updated[i] = false;
  }

  // Get the Unique ID from /etc/machine-id
  std::string unique_id;
  std::ifstream machine_id_file("/etc/machine-id");
  if (machine_id_file.good()) {
    std::getline(machine_id_file, unique_id);
    machine_id_file.close();
  } else {
    std::cout << "Could not open /etc/machine-id" << std::endl;
    return 1;
  }
  unique_id += "_simple_on_off_light_multiple_devices";

  // Create the connector
  auto connector =
      std::make_shared<MQTTConnector>(ip, port, username, password);

  // Create the devices
  for (int i = 0; i < _device_count; i++) {
    std::string device_name = _device_name_prefix + std::to_string(i);
    // By using lambda and capturing the value of i, we can create multiple
    // devices with different callbacks in a loop
    auto light = std::make_shared<OnOffLightDevice>(
        device_name, unique_id,
        [i](bool state) { controlStateCallback(i, state); });
    light->init();
    connector->registerDevice(light);
  }

  connector->connect();

  // Run the device
  int loop_count = 0;
  while (1) {
    // Process messages from the MQTT server for 1 second
    connector->processMessages(1000);

    // Every 10 seconds, change the state of the light
    if (loop_count % 10 == 0) {
      for (int i = 0; i < _device_count; i++) {
        _state[i] = !_state[i];
        _state_updated[i] = true;
      }
    }
    loop_count++;

    // Every second, check if there is an update to the state of the light, and
    // if so, update the state
    for (int i = 0; i < _device_count; i++) {
      if (_state_updated[i]) {
        LOG_INFO("Updating state for {}", i);
        auto light = std::dynamic_pointer_cast<OnOffLightDevice>(
            connector->getDevice(_device_name_prefix + std::to_string(i)));
        if (light) {

          light->set(_state[i]);
          _state_updated[i] = false;
        } else {
          LOG_ERROR("Could not find device {}", i);
        }
      }
    }
  }
}
