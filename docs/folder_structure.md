hass_mqtt_device/
│
├── include/
│   ├── YourLibraryName/
│   │   ├── core/
│   │   │   ├── mqtt_connector.h
│   │   │   ├── device_base.h
│   │   │   └── ...
│   │   ├── devices/
│   │   │   ├── light_device.h
│   │   │   ├── thermostat_device.h
│   │   │   ├── sensor_device.h
│   │   │   └── ...
│   │   └── utils/
│   │       ├── logger.hpp
│   │       └── ...
│
├── src/
│   ├── core/
│   │   ├── mqtt_connector.cpp
│   │   ├── device_base.cpp
│   │   └── ...
│   ├── devices/
│   │   ├── light_device.cpp
│   │   ├── thermostat_device.cpp
│   │   ├── sensor_device.cpp
│   │   └── ...
│   └── utils/
│       ├── logger.cpp
│       └── ...
│
├── tests/
│   ├── core/
│   │   ├── test_mqtt_connector.cpp
│   │   ├── test_device_base.cpp
│   │   └── ...
│   ├── devices/
│   │   ├── test_light_device.cpp
│   │   ├── test_thermostat_device.cpp
│   │   ├── test_sensor_device.cpp
│   │   └── ...
│   └── utils/
│       ├── test_logger.cpp
│       └── ...
│
├── docs/
│   ├── index.md
│   ├── core.md
│   ├── devices.md
│   └── ...
│
├── examples/
│   ├── light_example.cpp
│   ├── thermostat_example.cpp
│   └── ...
│
├── CMakeLists.txt
├── README.md
└── LICENSE
