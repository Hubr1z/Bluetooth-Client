# BLE Client for multipoint to point connection

## Summary of the Code
This is 1 of the 2 source codes used in this mmWave for human crowdedness measurement 3rd year project. The source code in this repository is responsible for scanning and connecting to sensor node UUIDs. It is capable of connecting to more than one server provided a new BLE device object is added to the source code along with the UUID to search for. The class has already been made. Creating a new client to connect to is a simple as calling a new object class in the `setup()` function and adding a new `if` statement in the `notifyCallback` callback function in `main.cpp` to be able to read the transmitted data from the newly added sensor node server. The client has a callback every time it is notified by the servers and modifies the stored data values accordingly. The data is used in the human crowdedness algorithm which is a simple switch statement as of right now. The position of the sensors are quite important for this system to work properly.


## How to Use
The mmWave source code is used on the ESP32C6 XIAO microcontroller using an arduino framework. This folder is self contained (assuming arduino framework used since it also includes the BLE libraries) and does not contain any imported libraries. It is recommended to use this folder on VSCode [PlatformIO](https://platformio.org) extension since it is the IDE used for this project complete with build and upload to the ESP32C6 microcontroller. Further guides on how to use PlaformIO with ESP32C6 found [here](https://wiki.seeedstudio.com/xiao_esp32c6_with_platform_io/). After setting up the IDE, just clone the repository to use.


Source code based on espressif arduino-esp32 BLE Client example code found [here](https://github.com/espressif/arduino-esp32/tree/master/libraries/BLE/examples/Client)
