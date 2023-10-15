# Library for communicating with Home Assistant

`hass_mqtt_device` is a C++ library designed to simplify creating a device and connecting it to a Home Assistant MQTT server.

There are a number of functions and device types available. See the examples for more info on how to use the library.

## Requirements

### Build requirements

- A modern C++ compiler with C++17 support.
- CMake version 3.10 or higher for building the project.
- Mosquitto MQTT broker development package

### Optional

- gTest

## Installation

1. Clone the repository:
```
git clone https://github.com/yourusername/hass_mqtt_device.git
```

2. Navigate to the project directory and create a build directory:
```
cd hass_mqtt_device
```

3. Configure and build the project:
```
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
