# hass_mqtt_device: MQTT Device Library for Home Assistant

`hass_mqtt_device` is a C++ library designed to simplify the connection to a Home Assistant MQTT server. It allows users to present themselves as various devices or a combination of multiple devices.

## Requirements

### System Requirements

- A modern C++ compiler with C++17 support.
- CMake version 3.10 or higher for building the project.

### Dependencies

- **Paho MQTT C++ Client**: This library relies on the Paho MQTT C++ Client for MQTT connectivity.
  - Installation instructions can be found on the [Paho MQTT C++ GitHub Repository](https://github.com/eclipse/paho.mqtt.cpp).

### Optional

- If you plan to run tests, ensure you have a testing framework (like Google Test) set up.

## Installation

1. Clone the repository:
```
git clone https://github.com/yourusername/hass_mqtt_device.git
```

2. Navigate to the project directory and create a build directory:
```
cd hass_mqtt_device
mkdir build
cd build
```

3. Configure and build the project:
```
cmake ..
make
```

4. (Optional) Install the library to your system:
```
sudo make install
```

## Usage

There are a few devices defined for comfort. See the devices folder. It is rather easy to make your own devices by inheriting from DeviceBase, then add functions from the functions folder, or create your own functions from FunctionBase.
### Examples

In the examples folder there are a few examples on how the library can be used.

## Contributing

If you'd like to contribute, please fork the repository and make changes as you'd like. Pull requests are warmly welcome.

## License

See the LICENSE file, but in short it is MIT.
