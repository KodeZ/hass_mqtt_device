/**
 * @author      Morgan Tørvolt
 * @contributors somebody, hopefully@someday.com
 * @copyright   See LICENSE file
 */

/**
 * This example shows how to create a device advanced device with for a rpi
 * with relays. The example assumes that the following product is used, but is
 * easily modified: https://smile.amazon.com/gp/product/B07JF4D814
 *
 * This example shows a hvac device used for a balanced ventilation system with
 * heat recovery. The device should be automatically discovered by Home Assistant.
 * It also includes 4 temperature sensors that on the rPi should be connected in
 * the usual 1-wire way.
 *
 * This example requires the wiringPi library to be installed. On raspian do:
 * sudo apt-get install wiringpi
 */

#include "hass_mqtt_device/core/device_base.h"
#include "hass_mqtt_device/core/mqtt_connector.h"
#include "hass_mqtt_device/devices/hvac.h"
#include "hass_mqtt_device/functions/number.h"
#include "hass_mqtt_device/functions/sensor.h"
#include "hass_mqtt_device/functions/sensor_attributes_factory.hpp"
#include "hass_mqtt_device/functions/switch.h"
#include "hass_mqtt_device/logger/logger.hpp"
#include "math.h"

#include <chrono> // for std::chrono::seconds
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <thread> // for std::this_thread::sleep_for
#if defined(ARM_ARCH) || defined(ARM64_ARCH)
#include <wiringPi.h>
#else
// Mock the wiringPi functions for non-arm platforms, usually PC for testing
// Print warning to the compiler output
#warning "Compiling for non-arm platform, using mock wiringPi functions"
#define OUTPUT 0
#define INPUT 0
#define PUD_UP 0
#define PUD_DOWN 0
void wiringPiSetup()
{
    LOG_DEBUG("wiringPiSetup called");
}
void digitalWrite(int pin, bool state)
{
    LOG_DEBUG("Relay {} set to {}", pin, state);
}
bool digitalRead(int pin)
{
    static bool state = false;
    state = !state;
    LOG_DEBUG("Relay {} read", pin);
    return state;
}
void pinMode(int pin, int mode)
{
    LOG_DEBUG("Relay {} set to mode {}", pin, mode);
}
void pullUpDnControl(int pin, int mode)
{
    LOG_DEBUG("Relay {} set to mode {}", pin, mode);
}
#endif

const int tick_size_ms = 1000;
bool stop_threads = false;
nlohmann::json config;

// Will be updated on every read cycle. Should be reset by the user in order to detect when a new read has happened
bool has_read_temp = false;

// Temperature reading thread
void tempReadingLoop()
{
    LOG_TRACE("Starting temp sensor thread");
    const std::string base_path = "/sys/bus/w1/devices";
    int temp_read_counter = 0;
    while(!stop_threads)
    {
        if(std::filesystem::is_directory(base_path))
        {
            LOG_DEBUG("Reading files");
            std::list<std::string> paths;
            // List all sensors
            for(const auto& entry : std::filesystem::directory_iterator(base_path))
            {
                if(!stop_threads && std::filesystem::is_directory(entry.path()) &&
                   std::filesystem::is_regular_file(entry.path() / "temperature"))
                {
                    // Get sensor id
                    if(entry.path().end() != entry.path().begin())
                    {
                        std::string sensor = *std::prev(entry.path().end());
                        // Read sensor values and store
                        std::ifstream infile(entry.path() / "temperature");
                        double temp = NAN;
                        infile >> temp;
                        if(!infile.good())
                        {
                            LOG_ERROR("Failed to read temperature from {}", sensor);
                            continue;
                        }
                        temp /= 1000;
                        if(temp == 85)
                        {
                            LOG_ERROR("Failed to read temperature from {}", sensor);
                            continue;
                        }
                        // Find function and store the value
                        for(auto& function : config["functions"])
                        {
                            if(function["type"] != "temp" || function["usage"]["type"] != "1w" ||
                               function["usage"]["id"] != sensor)
                            {
                                continue;
                            }
                            if(!function.contains("value") || (function.contains("value") && function["value"] != temp))
                            {
                                function["updated"] = true;
                            }
                            function["value"] = temp;
                            LOG_DEBUG("Sensor: {} Temp: {}", sensor, temp);
                            break;
                        }
                    }
                }
            }
            temp_read_counter++;
        }
        else
        {
            LOG_DEBUG("No 1w directory exist");
        }
        // Sleep for 10 seconds
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    LOG_INFO("Ending thread");
}

// Sanitize the config
int sanitizeConfig()
{
    // First check that the required fields are present
    if(!config.contains("ip") || !config.contains("port") || !config.contains("username") ||
       !config.contains("password") || !config.contains("functions") || !config.contains("status_file"))
    {
        LOG_ERROR("Config file does not contain the required fields");
        return 1;
    }
    // Check that the functions are valid
    for(const auto& function : config["functions"])
    {
        // Make sure that type, parameters, usage and name are present
        if(!function.contains("type") || !function.contains("parameters") || !function.contains("usage") ||
           !function.contains("name"))
        {
            LOG_ERROR("Function does not contain the required fields");
            return 1;
        }
        // If type is number, make sure that min, max and step is set in parameters, also make sure that usage type is either pwm or analog
        if(function["type"] == "number")
        {
            if(!function["parameters"].contains("min") || !function["parameters"].contains("max") ||
               !function["parameters"].contains("step"))
            {
                LOG_ERROR("Function of type number does not contain the required parameters");
                return 1;
            }
            if(!function["usage"].contains("gpio"))
            {
                LOG_ERROR("Function of type number does not contain the required gpio");
                return 1;
            }
            if(!function["usage"].contains("type") ||
               !(function["usage"]["type"] == "pwm" || function["usage"]["type"] == "analog"))
            {
                LOG_ERROR("Function of type number does not contain the required usage type");
                return 1;
            }
        }
        // If type is switch, make sure that usage type is gpio
        else if(function["type"] == "switch")
        {
            if(!function["usage"].contains("gpio"))
            {
                LOG_ERROR("Function of type switch does not contain the required gpio");
                return 1;
            }
            if(!function["usage"].contains("type") || function["usage"]["type"] != "onoff")
            {
                LOG_ERROR("Function of type switch does not contain the required usage type");
                return 1;
            }
        }
        // If type is temp, make sure that usage type is 1w, and that the usage id is set
        else if(function["type"] == "temp")
        {
            if(!function["usage"].contains("type") || function["usage"]["type"] != "1w")
            {
                LOG_ERROR("Function of type temp does not contain the required usage type");
                return 1;
            }
            if(!function["usage"].contains("id"))
            {
                LOG_ERROR("Function of type temp does not contain the required usage id");
                return 1;
            }
        }
    }
    return 0;
}

// Save the status to the status file if any of the values have changed
void saveStatus()
{
    bool changed = false;
    for(const auto& function : config["functions"])
    {
        if(!function.contains("value_saved") ||
           (function.contains("value") && function["value"] != function["value_saved"]))
        {
            changed = true;
            break;
        }
    }
    if(!changed)
    {
        LOG_DEBUG("No changes to save");
        return;
    }
    LOG_DEBUG("Saving state");
    auto status_file_name = config.at("status_file").get<std::string>();
    // Check if the folder exists, if not create it. If it fails, print error
    auto folder = status_file_name.substr(0, status_file_name.find_last_of("/"));
    try
    {
        if(!std::filesystem::exists(folder))
        {
            if(!std::filesystem::create_directories(folder))
            {
                LOG_ERROR("Folder for the status file does not exist. Could not create folder {}", folder);
                return;
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR("Error creating folder for status file: {}", e.what());
        return;
    }
    std::ofstream status_file(status_file_name);
    if(status_file.is_open())
    {
        nlohmann::json status_json;
        for(auto& function : config["functions"])
        {
            if(!function.contains("value"))
            {
                continue;
            }
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
        LOG_ERROR("Could not open status file for writing");
    }
}

void readStatus()
{
    LOG_TRACE("readStatus start");
    auto status_file_name = config.at("status_file").get<std::string>();
    std::ifstream status_file(status_file_name);
    if(status_file.good())
    {
        try
        {
            nlohmann::json status_json = nlohmann::json::parse(status_file);
            for(const auto& function : status_json["functions"])
            {
                if(function.contains("value") && function.contains("name"))
                {
                    LOG_DEBUG("Setting value for {} to {}", function["name"], function["value"]);

                    // Find name in config, and set value
                    for(auto& config_function : config["functions"])
                    {
                        if(config_function["name"] == function["name"])
                        {
                            config_function["value"] = function["value"];
                            config_function["value_saved"] = function["value"];
                            config_function["updated"] = true;
                            break;
                        }
                    }
                }
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
    LOG_TRACE("readStatus end");
}

void controlNumberCallback(int index, double number)
{
    if(number != config["functions"][index]["value"])
    {
        config["functions"][index]["value"] = number;
        config["functions"][index]["updated"] = true;
        LOG_INFO("number for index {} changed to {}", index, number);
    }
    else
    {
        LOG_INFO("number for index {} already set to {}", index, number);
    }
}

void controlSwitchCallback(int index, bool state)
{
    if(state != config["functions"][index]["value"])
    {
        config["functions"][index]["value"] = state;
        config["functions"][index]["updated"] = true;
        LOG_INFO("number for index {} changed to {}", index, state);

        bool value = config["functions"][index]["value"].get<bool>();
        // If an active state has been defined and it is false, invert the value
        if(config["functions"][index]["usage"].contains("active_state") &&
           !config["functions"][index]["usage"]["active_state"])
        {
            value = !value;
        }
        digitalWrite(config["functions"][index]["usage"]["gpio"], value);
    }
    else
    {
        LOG_INFO("number for index {} already set to {}", index, state);
    }
}

void updateNumberPwmOutputs()
{
    static int loop_count = 0;
    loop_count++;

    // To help with debugging, we write the status of the pwm outputs to a file
    std::ofstream status("/tmp/hass_mqtt_pwm");
    if(!status.good())
    {
        LOG_WARN("Could not open /tmp/hass_mqtt_pwm");
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

// Forward declare the special handling function
void specialHandling();

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

    // Read the config file if there is one
    LOG_DEBUG("Reading config file");

    std::ifstream config_file("/etc/hass_mqtt.json");
    if(!config_file.is_open())
    {
        LOG_ERROR("Could not open /etc/hass_mqtt.json");
        stop_threads = true;
        return 1;
    }
    try
    {
        LOG_DEBUG("Parsing JSON");
        config = nlohmann::json::parse(config_file);
    }
    catch(const nlohmann::json::exception& e)
    {
        LOG_ERROR("Error parsing JSON: {}", e.what());
        stop_threads = true;
        return 1;
    }
    config_file.close();

    // Sanity check the config file
    auto ret = sanitizeConfig();
    if(ret != 0)
    {
        LOG_ERROR("Config file is not valid");
        stop_threads = true;
        return ret;
    }

    // Read the status file if there is one
    readStatus();

    // Set the pinModes for the gpio pins
    LOG_DEBUG("Setting pin modes");

    wiringPiSetup();
    for(const auto& function : config["functions"])
    {
        // If gpio is in the usage, set the pin mode
        if(function["usage"].contains("gpio"))
        {
            // If the type is not input or analoginput, set the pin to output
            if(function["usage"]["type"] != "input")
            {
                pinMode(function["usage"]["gpio"], OUTPUT);
                // If pwm, set to !active_state
                if(function["usage"]["type"] == "pwm")
                {
                    digitalWrite(function["usage"]["gpio"], !function["usage"]["active_state"]);
                }
                // If switch, set to value xor active_state
                else if(function["usage"]["type"] == "onoff")
                {
                    if(function.contains("value") && function["value"].is_boolean())
                    {
                        bool value = function["value"].get<bool>();
                        // If an active state has been defined and it is false, invert the value
                        if(function["usage"].contains("active_state") && !function["usage"]["active_state"])
                        {
                            value = !value;
                        }
                        digitalWrite(function["usage"]["gpio"], value);
                    }
                    else
                    {
                        digitalWrite(function["usage"]["gpio"], !function["usage"]["active_state"]);
                    }
                }
                continue;
            }
            // If the type is input or analoginput, set the pin to input
            pinMode(function["usage"]["gpio"], INPUT);
            // If pull is defined and set to up or down, set it
            if(function["usage"].contains("pull"))
            {
                if(function["usage"]["pull"] == "up")
                {
                    pullUpDnControl(function["usage"]["gpio"], PUD_UP);
                }
                else if(function["usage"]["pull"] == "down")
                {
                    pullUpDnControl(function["usage"]["gpio"], PUD_DOWN);
                }
                else
                {
                    LOG_WARN("Unknown pull type {}", function["usage"]["pull"].get<std::string>());
                }
            }
        }
    }

    // Start the threads
    std::thread temp_thread(tempReadingLoop);

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
        LOG_ERROR("Could not open /etc/machine-id");
        stop_threads = true;
        return 1;
    }
    unique_id += "_rpi_relays_1w_temp";

    // Create the connector
    auto connector = std::make_shared<MQTTConnector>(config.at("ip").get<std::string>(),
                                                     config.at("port").get<int>(),
                                                     config.at("username").get<std::string>(),
                                                     config.at("password").get<std::string>(),
                                                     unique_id);

    // Create the device
    auto device = std::make_shared<DeviceBase>("Heating controls", "heating_controls");

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
        else if(function["type"] == "switch")
        {
            std::shared_ptr<SwitchFunction> func_ptr =
                std::make_shared<SwitchFunction>(function["name"],
                                                 [index](bool state) { controlSwitchCallback(index, state); });
            bool value = false;
            if(function.contains("value"))
            {
                value = function["value"].get<bool>();
            }
            else
            {
                // Making sure that there at least is a value
                function["value"] = value;
            }
            func_ptr->update(value);
            device->registerFunction(func_ptr);
        }
        else if(function["type"] == "temp")
        {
            std::shared_ptr<SensorFunction<double>> func_ptr =
                std::make_shared<SensorFunction<double>>(function["name"], getTemperatureSensorAttributes());
            if(function.contains("value"))
            {
                func_ptr->update(function["value"].get<double>());
            }
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
    LOG_TRACE("Registering device");
    connector->registerDevice(device);
    connector->connect();

    // Send the status
    LOG_TRACE("Sending intial status");
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
            saveStatus();
        }

        // Update the number pwm outputs
        updateNumberPwmOutputs();

        // Check if any values are updated, send the updated values on mqtt
        int i = 0;
        for(const auto& function : device->getFunctions())
        {
            if(config["functions"][i].contains("value") && config["functions"][i].contains("updated") &&
               config["functions"][i]["updated"] == true)
            {
                config["functions"][i]["updated"] = false;
                LOG_DEBUG("Updating function {} to {}", function->getName(), config["functions"][i].dump());
                // For number types, we need to update the value as a double
                if(config["functions"][i]["type"] == "number")
                {
                    std::dynamic_pointer_cast<NumberFunction>(function)->update(
                        config["functions"][i]["value"].get<double>());
                }
                // For switch types, we need to update the value as a bool
                else if(config["functions"][i]["type"] == "switch")
                {
                    std::dynamic_pointer_cast<SwitchFunction>(function)->update(
                        config["functions"][i]["value"].get<bool>());
                }
                // For temperature readings, we need to update the value as a double
                else if(config["functions"][i]["type"] == "temp")
                {
                    std::dynamic_pointer_cast<SensorFunction<double>>(function)->update(
                        config["functions"][i]["value"].get<double>());
                }
            }
            ++i;
        }

        specialHandling();

        // LOG_DEBUG("Config: {}", config.dump());

        // Process messages from the MQTT server for 1 second
        connector->processMessages(tick_size_ms);
    }
}

void specialHandling()
{
    // Find the "To houses combined" temperature. If above 43, turn off the switch named Varmepumpe. If below 40, turn on the switch named Varmepumpe
    double to_houses_combined = NAN;
    for(const auto& function : config["functions"])
    {
        if(function["type"] == "temp" && function["name"] == "To houses combined" && function.contains("value") &&
           function["value"].is_number())
        {
            to_houses_combined = function["value"].get<double>();
            break;
        }
    }
    if(std::isnan(to_houses_combined))
    {
        LOG_ERROR("Could not find the temperature sensor named To houses combined");
    }
    else
    {
        LOG_DEBUG("To houses combined: {}", to_houses_combined);
        for(auto& function : config["functions"])
        {
            if(function["type"] == "switch" && function["name"] == "Varmepumpe")
            {
                bool value = function["value"].get<bool>();
                if(to_houses_combined > 43 && value)
                {
                    function["value"] = false;
                    function["updated"] = true;
                    LOG_DEBUG("Turning off Varmepumpe");
                }
                else if(to_houses_combined < 40 && !value)
                {
                    function["value"] = true;
                    function["updated"] = true;
                    LOG_DEBUG("Turning off Varmepumpe");
                }
                break;
            }
        }
    }
    // Check if the temperature "Solar to collectors" against "Solar from collectors".
    // If it is more than 3 degrees warmer turn on the switch called Use solar, unless the To houses combined is above 75
    double temp_solar_to_collectors = NAN;
    double temp_solar_from_collectors = NAN;
    for(const auto& function : config["functions"])
    {
        if(function["type"] == "temp" && function["name"] == "Solar to collectors" && function.contains("value") &&
           function["value"].is_number())
        {
            temp_solar_to_collectors = function["value"].get<double>();
        }
        else if(function["type"] == "temp" && function["name"] == "Solar from collectors" &&
                function.contains("value") && function["value"].is_number())
        {
            temp_solar_from_collectors = function["value"].get<double>();
        }
        if(!std::isnan(temp_solar_to_collectors) && !std::isnan(temp_solar_from_collectors))
        {
            break;
        }
    }
    if(std::isnan(temp_solar_to_collectors) || std::isnan(temp_solar_from_collectors) || std::isnan(to_houses_combined))
    {
        LOG_ERROR("Could not find the temperature sensors named To houses combined, Solar to collectors and/or Solar "
                  "from collectors");
    }
    else
    {
        LOG_DEBUG("Solar to collectors:{} from:{}", temp_solar_to_collectors, temp_solar_from_collectors);
        for(auto& function : config["functions"])
        {
            if(function["type"] == "switch" && function["name"] == "Use solar")
            {
                bool value = function["value"].get<bool>();
                if(temp_solar_to_collectors - temp_solar_from_collectors > 3 && to_houses_combined < 75 && !value)
                {
                    function["value"] = true;
                    function["updated"] = true;
                    LOG_DEBUG("Turning on Use solar");
                }
                else if(temp_solar_to_collectors - temp_solar_from_collectors <= 3 && value)
                {
                    function["value"] = false;
                    function["updated"] = true;
                    LOG_DEBUG("Turning off Use solar");
                }
                break;
            }
        }
    }
}
