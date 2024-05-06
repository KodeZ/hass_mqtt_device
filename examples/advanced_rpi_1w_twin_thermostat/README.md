# Energy recovery ventilation

This example is a bit complex. It is using an rPi to control a valve and three relays in order to maintain a temperature. The temperature
is read from one-wire sensors. In this use case, there is one temp sensor at the input, one at the output, and one between the mixing valve and the electric heater.
The electric heater has three heater elements. We have a relay on two of them, and a solid state relay on one. The SSR is running a slow PWM with period of 5 seconds, and if it is running at 100% for more than a few minutes, we reset it to zero and turn on one of the other relays.


Install these packages at least:
sudo apt install nlohmann-json3-dev libspdlog-dev cmake
