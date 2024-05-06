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
#include "hass_mqtt_device/functions/sensor.h"
#include "hass_mqtt_device/functions/sensor_attributes_factory.hpp"
#include "hass_mqtt_device/logger/logger.hpp"
#include "math.h"

#include <chrono> // for std::chrono::seconds
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <thread> // for std::this_thread::sleep_for
#ifdef __arm__
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
const long electric_heater_period = 5;

bool stop_threads = false;
nlohmann::json config;
double heating_setpoint = 27;
double average_temp = heating_setpoint;
double hystreresis = 0.4;

const int R1 = 12;
const int R2 = 13;
const int R3 = 14;
const int Vclose = 8;
const int Vopen = 9;

const int VALVE_POSITION_MOTOR_DURATION = 3; // Input from shifter

const std::map<std::string, std::string> temp_sensors = {{"28-0417503c19ff", "Input"},
                                                         {"28-0417507da9ff", "Heat exchanger"},
                                                         {"28-0417507f00ff", "Output"}};

std::map<std::string, double> temp_temperatures = {{"Input", 30}, {"Heat exchanger", 31}, {"Output", 32}};

bool changed = false;
void controlStateCallback(std::shared_ptr<HvacFunction> function, HvacSupportedFeatures feature, std::string value)
{
    LOG_INFO("Control callback called. Feature: {}, value: {}", feature, value);
    if(feature == HvacSupportedFeatures::TEMPERATURE_CONTROL_HEATING)
    {
        function->updateHeatingSetpoint(std::stod(value));
        if(heating_setpoint != std::stod(value))
        {
            heating_setpoint = std::stod(value);
            changed = true;
        }
    }
    else
    {
        LOG_ERROR("Unknown feature: {}", feature);
    }
}

// Recovery rotor thread. This thread is needed for my setup, you probably
// don't need it, but rather should just turn on/off an output.
double electric_heater_value = 0;
void electricHeaterThread()
{
    LOG_DEBUG("Starting heater thread");
    int second_counter = 0;
    while(!stop_threads)
    {
        if(electric_heater_value == 0.0)
        {
            // Turn off R1, sleep and continue, nothing to do
            digitalWrite(R1, false);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        long sleep_ms = std::floor(electric_heater_period * 1000 * electric_heater_value);
        sleep_ms = std::max(sleep_ms, electric_heater_period * 1000);

        // Turn on R1
        digitalWrite(R1, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        // Turn off R1
        digitalWrite(R1, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(electric_heater_period * 1000 - sleep_ms));
    }
}

void heaterThread()
{
    LOG_DEBUG("Starting heater thread");
    int second_counter = 0;
    while(!stop_threads)
    {
        // Control the electric heater
        if(average_temp < (heating_setpoint + 0.5 + (hystreresis/2)))
        {
            electric_heater_value = std::min(1.0, electric_heater_value + 0.03);
        }
        else if(average_temp > (heating_setpoint + 0.5 - (hystreresis/2)))
        {
            electric_heater_value = std::max(0.0, electric_heater_value - 0.03);
        }

        // Control the valve, but do not add the 0.5 degrees to the setpoint, but rather subtract it
        if(average_temp < (heating_setpoint - 0.5 + (hystreresis/2)))
        {
            digitalWrite(Vclose, true);
            digitalWrite(Vopen, false);
            // Run for 3 seconds
            std::this_thread::sleep_for(std::chrono::seconds(VALVE_POSITION_MOTOR_DURATION));
            digitalWrite(Vclose, false);
        }
        else if(average_temp > (heating_setpoint - 0.5 - (hystreresis/2)))
        {
            digitalWrite(Vclose, false);
            digitalWrite(Vopen, true);
            // Run for 3 seconds
            std::this_thread::sleep_for(std::chrono::seconds(VALVE_POSITION_MOTOR_DURATION));
            digitalWrite(Vopen, false);
        }
        else
        {
            digitalWrite(Vclose, false);
            digitalWrite(Vopen, false);
        }

        // Sleep for 10 seconds between each iteration
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

// Will be updated on every read cycle. Should be reset by the user in order to detect when a new read has happened
bool has_read_temp = false;

// Temperature reading thread
void tempReadingLoop()
{
    LOG_INFO("Starting temp sensor thread");
    const std::string base_path = "/sys/bus/w1/devices";
    int temp_read_counter = 0;
    int valve_control_counter = 0;
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
                        if(infile.good())
                        {
                            temp /= 1000;
                            if(temp == 85)
                            {
                                LOG_ERROR("Failed to read temperature from {}", sensor);
                                continue;
                            }
                            auto sensor_name = temp_sensors.find(sensor);
                            if(sensor_name == temp_sensors.end())
                            {
                                LOG_WARN("Unknown sensor {}", sensor);
                                continue;
                            }
                            temp_temperatures[sensor_name->second] = temp;
                            LOG_DEBUG("Sensor: {} Temp: {}", sensor, temp);
                        }
                    }
                }
            }
        }
        else
        {
            LOG_DEBUG("No 1w directory exist");
        }
        // Give the system 30 seconds to create the 1W system folder before sending default data
        if(temp_read_counter++ > 3)
        {
            has_read_temp = true;
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
    return 0;
}

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

    // Read the status file
    if(config.find("status_file") != config.end())
    {
        LOG_DEBUG("Reading status file");
        // If there is not a setting for status_file, we just ignore it
        auto status_file_name = config.at("status_file").get<std::string>();
        try
        {
            std::ifstream status_file(status_file_name);
            if(status_file.good())
            {
                int i = 0;
                status_file >> heating_setpoint;
                status_file.close();
            }
            else
            {
                LOG_WARN("Could not open status file to read start values {}", status_file_name);
            }
        }
        catch(const std::ifstream::failure& e)
        {
            LOG_ERROR("Error parsing JSON: {}", e.what());
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
    unique_id += "_hass_mqtt_twin_thermostat";

    // Create the connector
    auto connector = std::make_shared<MQTTConnector>(config.at("ip").get<std::string>(),
                                                     config.at("port").get<int>(),
                                                     config.at("username").get<std::string>(),
                                                     config.at("password").get<std::string>(),
                                                     unique_id);

    wiringPiSetup();
    pinMode(R1, OUTPUT);
    pinMode(R1, OUTPUT);
    pinMode(R1, OUTPUT);
    pinMode(Vclose, OUTPUT);
    pinMode(Vopen, OUTPUT);

    double electricHeaterValue = 0;

    // Start the threads
    std::thread electric_heater_thread(electricHeaterThread);
    std::thread heater_thread(heaterThread);
    std::thread temp_thread(tempReadingLoop);

    // Create the ventilator device
    auto thermostat = std::make_shared<HvacDevice>("Floor heating setpoint", "floor_heating_setpoint");
    thermostat->init([thermostat](HvacSupportedFeatures feature,
                                  std::string value) { controlStateCallback(thermostat->getFunction(), feature, value); },
                     HvacSupportedFeatures::TEMPERATURE | HvacSupportedFeatures::TEMPERATURE_CONTROL_HEATING,
                     {},
                     {},
                     {},
                     {});
    connector->registerDevice(thermostat);

    // Create the temperature sensors
    auto temperatures = std::make_shared<DeviceBase>("House temperatures", "temp");
    SensorAttributes attributes = getTemperatureSensorAttributes();
    for(const auto& [sensor_id, sensor_name] : temp_sensors)
    {
        auto temp = std::make_shared<SensorFunction<double>>(sensor_name, attributes);
        temperatures->registerFunction(temp);
    }
    connector->registerDevice(temperatures);

    // Connect to the mqtt server
    connector->connect();

    thermostat->getFunction()->updateHeatingSetpoint(heating_setpoint);

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
        if(loop_count % (2 * 60 * (1000 / tick_size_ms)) == 0 && changed)
        {
            LOG_DEBUG("Saving state");
            auto status_file_name = config.at("status_file").get<std::string>();
            std::ofstream status_file(status_file_name);
            if(status_file.good())
            {
                status_file << heating_setpoint << std::endl;
                status_file.close();
                changed = false;
            }
            else
            {
                LOG_WARN("Could not open status file to write current status {}", status_file_name);
            }
        }

        // Update the temperature
        if(has_read_temp)
        {
            has_read_temp = false;
            // Find the average temperature between input and output
            average_temp = (temp_temperatures["Input"] + temp_temperatures["Output"]) / 2;
            thermostat->getFunction()->updateTemperature(average_temp);
            for(const auto& [sensor_id, sensor_name] : temp_sensors)
            {
                auto temp = std::dynamic_pointer_cast<SensorFunction<double>>(temperatures->findFunction(sensor_name));
                if(temp)
                {
                    temp->update(temp_temperatures[sensor_name]);
                }
            }
        }

        // Process messages from the MQTT server for 1 second
        connector->processMessages(tick_size_ms);
    }
}
