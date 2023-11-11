# Energy recovery ventilation

This example is a bit more complex. It is being used with https://smile.amazon.com/gp/product/B07JF4D814. Some of the relays are set to control an energy recovery balanced ventilation. It has two main functions, energy recovery and fan speed. The fan speed is determined via two relays. The first relay will either activate the low speed, or connect power to the second relay, which two outputs will be medium or high speed.
The energy recovery is an on/off function. For this example there is an input and an output. This due to my energy recovery has a faulty timer relay, so I am doing this programmatically rather than just turning this on and off. Feel free to replace my abstraction here.

Install these packages at least:
sudo apt install nlohmann-json3-dev libspdlog-dev cmake
