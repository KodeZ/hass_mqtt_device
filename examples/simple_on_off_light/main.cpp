/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

#include "hass_mqtt_device/core/mqtt_connector.h"
#include "hass_mqtt_device/devices/on_off_light.h"
#include <fstream>
#include <iostream>
#include <memory>

bool _state = false;
bool _state_updated = true;

void setStateCallback(bool state) {
  std::cout << "State set to " << state << std::endl;
  if (state != _state) {
    _state = state;
    _state_updated = true;
    std::cout << "State changed to " << state << std::endl;
  } else {
    std::cout << "State unchanged" << std::endl;
  }
}

int main() {
  // Create a light device with the name "light" and the unique id grabbed from
  // /etc/machine-id
  std::string unique_id;
  std::ifstream machine_id_file("/etc/machine-id");
  if (machine_id_file.good()) {
    std::getline(machine_id_file, unique_id);
    machine_id_file.close();
  } else {
    std::cout << "Could not open /etc/machine-id" << std::endl;
    return 1;
  }

  // Create the device
  auto light =
      std::make_shared<OnOffLightDevice>("light", unique_id, setStateCallback);

  MQTTConnector connector("10.1.1.21", 1883,
                          "hass_mqtt_device_on_off_light_example", "password");
  connector.registerDevice(light);

  // Run the device
  while (1) {
    // Process messages from the MQTT server for 1 second
    connector.processMessages(1000);

    // Every second, check if there is an update to the state of the light, and
    // if so, update the state
    if (_state_updated) {
      light->setState(_state);
      _state_updated = false;
    }
  }
}
