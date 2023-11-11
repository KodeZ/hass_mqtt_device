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

#include "hass_mqtt_device/core/mqtt_connector.h"
#include "hass_mqtt_device/devices/hvac.h"
#include "hass_mqtt_device/devices/temp_sensor.h"
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
const int ROTATION_DELAY = 10;        // Seconds
const int RECOVER_ROTOR = 21;         // Recovery output
const int SPEED_LOW = 22;             // Low-other speed
const int SPEED_MED_HIGH = 23;        // Medium-High speed (if not Low on CH2)
const int RECOVER_ROTOR_POSITION = 3; // Input from shifter
const std::vector<std::string> DEVICE_MODES = {"cool", "heat"};
const std::vector<std::string> FAN_MODES = {"low", "medium", "high"};
const std::map<std::string, std::string> temp_sensors = {{"Incoming", "28-000004d00985" },
                                                         {"Outgoing", "28-000004d0531f" },
                                                         {"In/out 1", "28-000004ef1f39" },
                                                         {"In/out 2", "28-000004ef81bd" }};
std::map<std::string, double> temp_temperatures = {{"Incoming", NAN},
                                                   {"Outgoing", NAN},
                                                   {"In/out 1", NAN},
                                                   {"In/out 2", NAN}};

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

void controlStateCallback(std::shared_ptr<HvacFunction> function, HvacSupportedFeatures feature, std::string value)
{
    switch(feature)
    {
        case HvacSupportedFeatures::MODE_CONTROL:
            LOG_INFO("Power control: {}", value);
            if(value == "heat")
            {
                recovery_enabled = true;
                function->updateDeviceMode("heat");
            }
            else
            {
                recovery_enabled = false;
                function->updateDeviceMode("cool");
            }
            break;
        case HvacSupportedFeatures::FAN_MODE:
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
    LOG_INFO("Starting recovery rotor thread");
    int second_counter = 0;
    while(!stop_threads)
    {
        if(!recovery_enabled)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        if(++second_counter <= ROTATION_DELAY)
        {
            continue;
        }
        second_counter = 0;

        // Start recovery rotation
        digitalWrite(RECOVER_ROTOR, false);
        while(digitalRead(RECOVER_ROTOR_POSITION) == false)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        // Let the sensor stabilize
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // Wait until it returns
        while(digitalRead(RECOVER_ROTOR_POSITION) == true)
        {
            // We want to check this a bit more often
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        digitalWrite(RECOVER_ROTOR, true);
        // Sleep for the alotted time
        std::this_thread::sleep_for(std::chrono::seconds(ROTATION_DELAY));
    }
}

// Will be updated on every read cycle. Should be reset by the user in order to detect when a new read has happened
bool has_read_temp = false;
// Temperature reading thread
void tempReadingLoop()
{
    LOG_DEBUG("Starting temp sensor thread");
    const std::string base_path = "/sys/bus/w1/devices";
    int temp_read_counter = 0;
    int valve_control_counter = 0;
    while(!stop_threads)
    {
        if(++temp_read_counter % 100 == 0) // Loop this for 20 seconds
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
                                temp_temperatures[sensor] = temp;
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
        }
        // Sleep for 10 seconds
        has_read_temp = true;
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    LOG_DEBUG("Ending thread");
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

#ifdef __arm__
    wiringPiSetup();
#endif

    // Read the status file

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

    // Create the ventilator device
    auto ventilator = std::make_shared<HvacDevice>("house_ventilation", unique_id+"_hvac");
    ventilator->init([ventilator](HvacSupportedFeatures feature,
                  std::string value) { controlStateCallback(ventilator->getFunction(), feature, value); },
             HvacSupportedFeatures::TEMPERATURE | HvacSupportedFeatures::FAN_MODE | HvacSupportedFeatures::MODE_CONTROL,
             DEVICE_MODES,
             FAN_MODES,
             {},
             {});


    // Create the temperature sensors
    auto incoming_temp = std::make_shared<TemperatureSensorDevice>("incoming_air_temp", unique_id+"_temp");
    auto outgoing_temp = std::make_shared<TemperatureSensorDevice>("outgoing_air_temp", unique_id+"_temp");
    auto io1_temp = std::make_shared<TemperatureSensorDevice>("io1_air_temp", unique_id+"_temp");
    auto io2_temp = std::make_shared<TemperatureSensorDevice>("io2_air_temp", unique_id+"_temp");
    // Init the sensors
    incoming_temp->init();
    outgoing_temp->init();
    io1_temp->init();
    io2_temp->init();

    // Create the connector
    auto connector = std::make_shared<MQTTConnector>(ip, port, username, password);

    // Create the device
    connector->registerDevice(ventilator);
    connector->registerDevice(incoming_temp);
    connector->registerDevice(outgoing_temp);
    connector->registerDevice(io1_temp);
    connector->registerDevice(io2_temp);
    connector->connect();

    ventilator->getFunction()->updateDeviceMode("heat");
    ventilator->getFunction()->updateFanMode("low");

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
        }

        // Update the temperature
        if(has_read_temp)
        {
            has_read_temp = false;
            ventilator->getFunction()->updateTemperature(temp_temperatures[temp_sensors.at("Outgoing")]);
            incoming_temp->update(temp_temperatures[temp_sensors.at("Incoming")]);
            outgoing_temp->update(temp_temperatures[temp_sensors.at("Outgoing")]);
            io1_temp->update(temp_temperatures[temp_sensors.at("In/out 1")]);
            io2_temp->update(temp_temperatures[temp_sensors.at("In/out 2")]);
        }

        // Process messages from the MQTT server for 1 second
        connector->processMessages(tick_size_ms);
    }
}
