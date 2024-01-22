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
#endif

const int tick_size_ms = 1000;

bool stop_threads = false;
bool recovery_enabled = true;
const int ROTATION_DELAY = 45;        // Seconds

const int RECOVER_ROTOR = 21;         // Recovery output pin
const int SPEED_LOW = 22;             // Low-other speed pin
const int SPEED_MED_HIGH = 23;        // Medium-High speed (if not Low on CH2) pin

const int RECOVER_ROTOR_POSITION = 3; // Input from shifter

const std::vector<std::string> DEVICE_MODES = {"cool", "heat"};
const std::vector<std::string> FAN_MODES = {"low", "medium", "high"};

const std::map<std::string, std::string> temp_sensors = {{"28-000004d00985", "From house"},
                                                         {"28-000004ef1f39", "To house"},
                                                         {"28-0621b47f1183", "In/out 1"},
                                                         {"28-000004ef81bd", "In/out 2"}};
std::map<std::string, double> temp_temperatures = {{"From house", 20},
                                                   {"To house", 21.1},
                                                   {"In/out 1", 10},
                                                   {"In/out 2", 9.9}};

// Set fan speeds
void setFanSpeed(std::string speed)
{
    LOG_INFO("Setting fan speed to {}", speed);
    if(speed == "low")
    {
        digitalWrite(SPEED_LOW, true);
        digitalWrite(SPEED_LOW, true);
    }
    else if(speed == "medium")
    {
        digitalWrite(SPEED_LOW, false);
        digitalWrite(SPEED_MED_HIGH, true);
    }
    else if(speed == "high")
    {
        digitalWrite(SPEED_LOW, false);
        digitalWrite(SPEED_MED_HIGH, false);
    }
    else
    {
        LOG_WARN("Unknown fan speed {}", speed);
    }
}

bool changed = false; // Used to trigger writing new status file
void controlStateCallback(std::shared_ptr<HvacFunction> function, HvacSupportedFeatures feature, std::string value)
{
    switch(feature)
    {
        case HvacSupportedFeatures::MODE_CONTROL:
            LOG_INFO("Power control: {}", value);
            if(value == "heat")
            {
                if(!recovery_enabled)
                {
                    changed = true;
                }
                recovery_enabled = true;
                function->updateDeviceMode("heat");
            }
            else
            {
                if(recovery_enabled)
                {
                    changed = true;
                }
                recovery_enabled = false;
                function->updateDeviceMode("cool");
            }
            break;
        case HvacSupportedFeatures::FAN_MODE:
            if(function->getFanMode() != value)
            {
                changed = true;
            }
            setFanSpeed(value);
            function->updateFanMode(value);
            break;
        default:
            LOG_WARN("Unknown feature: {}", feature);
            break;
    }
}

// Recovery rotor thread. This thread is needed for my setup, you probably
// don't need it, but rather should just turn on/off an output.
void recoveryRotorThread()
{
    LOG_DEBUG("Starting recovery rotor thread");
    int second_counter = 0;
    while(!stop_threads)
    {
        if(!recovery_enabled)
        {
            LOG_DEBUG("Cooling mode");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        LOG_DEBUG("Heating mode");

        LOG_DEBUG("Starting recovery rotor rotation");
        // Start recovery rotation
        digitalWrite(RECOVER_ROTOR, false);
        while(digitalRead(RECOVER_ROTOR_POSITION) == false)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        LOG_DEBUG("Recovery rotor stabilized");
        // Let the sensor stabilize
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // Wait until it returns
        while(digitalRead(RECOVER_ROTOR_POSITION) == true)
        {
            // We want to check this a bit more often
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        digitalWrite(RECOVER_ROTOR, true);
        LOG_DEBUG("Ending recovery rotor rotation");

        // Sleep for the alotted time
        std::this_thread::sleep_for(std::chrono::seconds(ROTATION_DELAY));
    }
    LOG_DEBUG("Ending recovery rotor thread");
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

    std::string ip;
    int port = 0;
    std::string username;
    std::string password;

    // Read the config file if there is one
    LOG_DEBUG("Reading config file");

    nlohmann::json config;
    std::ifstream config_file("/etc/hass_mqtt.json");
    if(!config_file.is_open())
    {
        LOG_INFO("Could not open /etc/hass_mqtt.json");
    }
    else
    {
        try
        {
            LOG_DEBUG("Parsing JSON");
            config = nlohmann::json::parse(config_file);
            ip = config["ip"].get<std::string>();
            port = config["port"].get<int>();
            username = config["username"].get<std::string>();
            password = config["password"].get<std::string>();
        }
        catch(const nlohmann::json::exception& e)
        {
            LOG_ERROR("Error parsing JSON: {}", e.what());
        }
    }
    config_file.close();

    // Check and read the arguments
    if(argc >= 5)
    {
        std::string ip = argv[1];
        int port = std::stoi(argv[2]);
        std::string username = argv[3];
        std::string password = argv[4];
    }

    LOG_DEBUG("Parameters: ip: {}, port: {}, username: {}, password: {}", ip, port, username, password);

    std::string start_mode = "heat";
    std::string start_fan_mode = "low";
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
                status_file >> start_mode;
                status_file >> start_fan_mode;
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

#ifdef __arm__
    wiringPiSetup();
    pinMode(RECOVER_ROTOR, OUTPUT);
    pinMode(SPEED_LOW, OUTPUT);
    pinMode(SPEED_MED_HIGH, OUTPUT);
    pinMode(RECOVER_ROTOR_POSITION, INPUT);
    pullUpDnControl(RECOVER_ROTOR_POSITION, PUD_UP);
#endif

    recovery_enabled = true;
    setFanSpeed(start_fan_mode);

    // Start the threads
    std::thread recovery_thread(recoveryRotorThread);
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
    unique_id += "_rpi_energy_recovery_ventilation";

    // Create the connector
    auto connector = std::make_shared<MQTTConnector>(ip, port, username, password, unique_id);

    // Create the ventilator device
    auto ventilator = std::make_shared<HvacDevice>("House ventilation", "hvac");
    ventilator->init([ventilator](HvacSupportedFeatures feature,
                                  std::string value) { controlStateCallback(ventilator->getFunction(), feature, value); },
                     HvacSupportedFeatures::TEMPERATURE | HvacSupportedFeatures::FAN_MODE |
                         HvacSupportedFeatures::MODE_CONTROL,
                     DEVICE_MODES,
                     FAN_MODES,
                     {},
                     {});
    connector->registerDevice(ventilator);

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

    ventilator->getFunction()->updateDeviceMode(start_mode);
    ventilator->getFunction()->updateFanMode(start_fan_mode);

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
                status_file << ventilator->getFunction()->getDeviceMode() << std::endl;
                status_file << ventilator->getFunction()->getFanMode() << std::endl;
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
            ventilator->getFunction()->updateTemperature(temp_temperatures["From house"]);
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
