/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

/**
 * This example shows how to create a rpi_pwm advanced device with for a rpi
 * with relays. The example assumes that the following product is used, but is
 * easily modified: https://smile.amazon.com/gp/product/B07JF4D814 The board has
 * a silly pinout connectivity, so we need a translation table for the relays.
 *
 * This example requires the wiringPi library to be installed. On raspian do:
 * sudo apt-get install wiringpi
 */

#include "hass_mqtt_device/core/mqtt_connector.h"
#include "hass_mqtt_device/functions/number.h"
#include "hass_mqtt_device/logger/logger.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#ifdef __arm__
#include <wiringPi.h>
#else
// Mock the wiringPi functions for non-arm platforms, usually PC for testing
void digitalWrite(int pin, bool state) {
  LOG_DEBUG("Relay {} set to {}", pin, state);
}
#endif

int _relay_count = 8;
std::vector<int> _relay_pins = {21, 22, 23, 27, 24, 28, 29, 25};
std::vector<double> _relay_values = {0, 0, 0, 0, 0, 0, 0, 0};
std::vector<double> _relay_values_saved = {0, 0, 0, 0, 0, 0, 0, 0};
bool _updated = false;
const std::string _function_name_prefix = "rpi_relays_pwm_";
const int _pwm_period = 600; // 10 minutes
const bool active_pin_state = false;

void controlCallback(int relay, double number) {
  if (number != _relay_values[relay]) {
    _relay_values[relay] = number;
    _updated = true;
    LOG_INFO("number changed to {}", number);
  } else {
    LOG_INFO("number already set to {}", number);
  }
}

void updateOutputs();

// The main function.
int main(int argc, char *argv[]) {
  INIT_LOGGER_DEBUG();
#ifdef __arm__
  wiringPiSetup();
#endif

  // Read ip, port, username and password from a config file in
  // /etc/rpi_relays.conf The config file should look like this:
  // ip x.x.x.x
  // port y
  // username hass
  // password secret

  // Read the config file as json
  std::string ip;
  int port;
  std::string username;
  std::string password;
  std::string status_file;
  std::vector<std::string> names;

  std::ifstream config_file("/etc/rpi_relays.json");
  if (!config_file.is_open()) {
    std::cerr << "Could not open /etc/rpi_relays.json" << std::endl;
    return 1;
  }

  try {
    nlohmann::json config = nlohmann::json::parse(config_file);
    ip = config.at("ip").get<std::string>();
    port = config.at("port").get<int>();
    username = config.at("username").get<std::string>();
    password = config.at("password").get<std::string>();
    status_file = config.at("status_file").get<std::string>();
    for (auto name : config.at("names")) {
      names.push_back(name.get<std::string>());
    }
    if (names.size() != _relay_count) {
      std::cerr
          << "Number of names in config file does not match number of relays"
          << std::endl;
      return 1;
    }
  } catch (const nlohmann::json::exception &e) {
    std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    return 1;
  }

  // Read the status file
  std::ifstream status(status_file);
  if (status.is_open()) {
    std::string line;
    int i = 0;
    while (std::getline(status, line)) {
      try {
        _relay_values[i++] = std::stod(line);
        _relay_values_saved[i] = _relay_values[i];
      } catch (const std::exception &e) {
        std::cerr << "Error parsing status file: " << e.what() << std::endl;
        return 1;
      }
    }
    status.close();
  } else {
    std::cerr << "Could not open status file" << std::endl;
  }

  config_file.close();

  // Get a unique ID
  std::string unique_id;
  std::ifstream machine_id_file("/etc/machine-id");
  if (machine_id_file.good()) {
    std::getline(machine_id_file, unique_id);
    machine_id_file.close();
  } else {
    std::cout << "Could not open /etc/machine-id" << std::endl;
    return 1;
  }
  unique_id += "_rpi_relays_pwm";

  // Create the connector
  auto connector =
      std::make_shared<MQTTConnector>(ip, port, username, password);

  // Create the device
  auto rpi_pwm = std::make_shared<DeviceBase>("rpi_relays_slow_pwm", unique_id);

  // Create the functions
  for (int i = 0; i < _relay_count; i++) {
    std::string function_name = _function_name_prefix + std::to_string(i);
    if (names.size() > i) {
      function_name = names[i];
    }
    std::shared_ptr<NumberFunction> number_func =
        std::make_shared<NumberFunction>(
            function_name, [i](double state) { controlCallback(i, state); });
    number_func->update(_relay_values[i]);
    rpi_pwm->registerFunction(number_func);
  }

  // Register the device
  connector->registerDevice(rpi_pwm);
  connector->connect();

  rpi_pwm->sendStatus();

  // Run the device
  int loop_count = 0;
  while (1) {
    ++loop_count;
    // If we have been running for 2 minutes, save the state (if changed)
    if (loop_count % (2 * 60) == 0) {
      bool changed = false;
      for (int i = 0; i < _relay_count; i++) {
        if (_relay_values[i] != _relay_values_saved[i]) {
          changed = true;
          break;
        }
      }
      if (changed) {
        LOG_DEBUG("Saving state");
        std::ofstream status(status_file);
        if (status.is_open()) {
          for (int i = 0; i < _relay_count; i++) {
            status << _relay_values[i] << std::endl;
            _relay_values_saved[i] = _relay_values[i];
          }
          status.close();
        } else {
          std::cerr << "Could not open status file" << std::endl;
        }
      }
    }

    // Process messages from the MQTT server for 1 second
    connector->processMessages(1000);

    // Every second, check if there is an update
    if (_updated) {
      // Send status for all functions
      _updated = false;

      int i = 0;
      for (auto function : rpi_pwm->getFunctions()) {
        std::dynamic_pointer_cast<NumberFunction>(function)->update(
            _relay_values[i++]);
      }
    }

    // Update the outputs
    updateOutputs();
  }
}

void updateOutputs() {
  static int loop_count = 0;
  loop_count++;

  for (int i = 0; i < _relay_count; i++) {
    if (_relay_values[i] > 0) {
      if ((loop_count + (i * loop_count / _relay_count)) % _pwm_period <
          (_relay_values[i] * _pwm_period) / 100) {
        digitalWrite(_relay_pins[i], active_pin_state);
      } else {
        digitalWrite(_relay_pins[i], !active_pin_state);
      }
    } else {
      digitalWrite(_relay_pins[i], !active_pin_state);
    }
  }
}
