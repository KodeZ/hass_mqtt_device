/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

/**
 * This example shows how to create a device advanced device with for a rpi
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
void digitalWrite(int pin, bool state)
{
    LOG_DEBUG("Relay {} set to {}", pin, state);
}
#endif

const int tick_size_ms = 1000;

// Making the config global to avoid passing it around
nlohmann::json config;

bool _updated = false;

void controlNumberCallback(int index, double number)
{
    if(number != config["functions"][index]["value"])
    {
        config["functions"][index]["value"] = number;
        _updated = true;
        LOG_INFO("number for index {} changed to {}", index, number);
    }
    else
    {
        LOG_INFO("number for index {} already set to {}", index, number);
    }
}

void updateNumberPwmOutputs();

// The main function.
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

#ifdef __arm__
    wiringPiSetup();
#endif

    // Read the config file
    LOG_DEBUG("Reading config file");
    std::ifstream config_file("/etc/rpi_relays.json");
    if(!config_file.is_open())
    {
        LOG_ERROR("Could not open /etc/rpi_relays.json");
        return 1;
    }

    try
    {
        config = nlohmann::json::parse(config_file);
    }
    catch(const nlohmann::json::exception& e)
    {
        LOG_ERROR("Error parsing JSON: {}", e.what());
        return 1;
    }
    config_file.close();

    // Read the status file
    {
        LOG_DEBUG("Reading status file");
        auto status_file_name = config.at("status_file").get<std::string>();
        std::ifstream status_file(status_file_name);
        if(status_file.good())
        {
            int i = 0;
            try
            {
                nlohmann::json status_json = nlohmann::json::parse(status_file);
                for(const auto& function : status_json["functions"])
                {
                    if(function.contains("value"))
                    {
                        config["functions"][i]["value"] = function["value"];
                    }
                    ++i;
                }
            }
            catch(const nlohmann::json::exception& e)
            {
                LOG_ERROR("Error parsing JSON: {}", e.what());
            }
            status_file.close();
        }
        else
        {
            std::cerr << "Could not open status file" << std::endl;
        }
    }

    // Get a unique ID
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
    unique_id += "_rpi_relays_pwm";

    // Create the connector
    auto connector = std::make_shared<MQTTConnector>(config.at("ip").get<std::string>(),
                                                     config.at("port").get<int>(),
                                                     config.at("username").get<std::string>(),
                                                     config.at("password").get<std::string>());

    // Create the device
    auto device = std::make_shared<DeviceBase>("rpi_relays_slow_pwm", unique_id);

    // Create the functions
    int index = 0;
    for(auto& function : config["functions"])
    {
        if(function["type"] == "number")
        {
            std::shared_ptr<NumberFunction> func_ptr = std::make_shared<NumberFunction>(
                function["name"],
                [index](double state) { controlNumberCallback(index, state); },
                function["parameters"]["max"].get<double>(),
                function["parameters"]["min"].get<double>(),
                function["parameters"]["step"].get<double>());
            auto value = function["parameters"]["min"].get<double>();
            if(function.contains("value"))
            {
                value = function["value"].get<double>();
            }
            else
            {
                // Making sure that there at least is a value
                function["value"] = value;
            }
            func_ptr->update(value);
            device->registerFunction(func_ptr);
        }
        else
        {
            LOG_WARN("Unknown function type {}", function["type"].get<std::string>());
            return 1;
        }
        ++index;
    }

    // Register the device
    connector->registerDevice(device);
    connector->connect();

    device->sendStatus();

    // Run the device
    // Here we loop forever, and basically handle incoming messages and update
    // the output registers. Every second we also update the outputs accordingly.
    // Every 2 minutes we check if any of the settings have changed, and if so
    // we save the state to the status file.
    int loop_count = 0;
    while(true)
    {
        ++loop_count;
        // If we have been running for 2 minutes, save the state (if changed)
        if(loop_count % (2 * 60 * (1000 / tick_size_ms)) == 0)
        {
            bool changed = false;
            for(const auto& function : config["functions"])
            {
                if(!function.contains("value_saved") ||
                   (function.contains("value") && function["value"] != function["values_saved"]))
                {
                    changed = true;
                    break;
                }
            }
            if(changed)
            {
                LOG_DEBUG("Saving state");
                auto status_file_name = config.at("status_file").get<std::string>();
                std::ofstream status_file(status_file_name);
                if(status_file.is_open())
                {
                    nlohmann::json status_json;
                    for(auto& function : config["functions"])
                    {
                        function["value_saved"] = function["value"];
                        nlohmann::json function_json;
                        function_json["name"] = function["name"];
                        if(function.contains("value"))
                        {
                            function_json["value"] = function["value"];
                        }
                        status_json["functions"].push_back(function_json);
                    }
                    status_file << status_json.dump(4);
                    status_file.close();
                }
                else
                {
                    std::cerr << "Could not open status file" << std::endl;
                }
            }
        }

        // Every loop, check if there is an update
        if(_updated)
        {
            // Send status for all functions
            _updated = false;

            int i = 0;
            for(const auto& function : device->getFunctions())
            {
                if(config["functions"][i].contains("value"))
                {
                    LOG_INFO("Updating function {} to {}", function->getName(), config["functions"][i].dump());
                    // For number types, we need to update the value as a double
                    if(config["functions"][i]["type"] == "number")
                    {
                        std::dynamic_pointer_cast<NumberFunction>(function)->update(
                            config["functions"][i]["value"].get<double>());
                    }
                }
                ++i;
            }
        }

        // Update the outputs
        updateNumberPwmOutputs();

        // Process messages from the MQTT server for 1 second
        connector->processMessages(tick_size_ms);
    }
}

void updateNumberPwmOutputs()
{
    static int loop_count = 0;
    loop_count++;

    std::ofstream status("/tmp/rpi_relays_pwm");
    if(!status.good())
    {
        LOG_WARN("Could not open /tmp/rpi_relays_pwm");
    }

    int i = 0;
    for(auto& function : config["functions"])
    {
        // We only care about number functions here
        if(function["type"] != "number" || function["usage"]["type"] != "pwm")
        {
            continue;
        }
        if(function["value"] > 0)
        {
            int count_offset = function["usage"]["offset"].get<int>() / tick_size_ms;
            int period = function["usage"]["period"].get<int>() / tick_size_ms;
            if((loop_count + count_offset) % period <
               (function["value"].get<double>() * period) / function["parameters"]["max"].get<double>())
            {
                digitalWrite(function["usage"]["gpio"], function["usage"]["active_state"]);
                function["state"] = true;
            }
            else
            {
                digitalWrite(function["usage"]["gpio"], !function["usage"]["active_state"]);
                function["state"] = false;
            }
        }
        else
        {
            digitalWrite(function["usage"]["gpio"], !function["usage"]["active_state"]);
            function["state"] = false;
        }
        if(status.good())
        {
            status << function["usage"]["gpio"] << " " << function["state"] << " " << function["name"] << std::endl;
        }
        ++i;
    }

    if(status.is_open())
    {
        status.close();
    }
}
