#include <iostream>
#include <vector>
#include <fstream>
#include <experimental/filesystem>

#include "SerialPortConfigurationGenerator.h"
#include "SerialPortDiscover.h"
#include "SerialPortIdentification.h"

namespace fs = std::experimental::filesystem;

int main() {
    std::string dev = "/dev";
    SerialPortDiscover serialPortDiscover;
    SerialPortIdentification serialPortIdentification(serialPortDiscover);
    SerialPortConfigurationGenerator serialPortConfigurationGenerator;

    auto devices = serialPortIdentification.ListFilesOfType(dev, std::string("ACM"));

    auto connectedDevices = serialPortIdentification.ConnectedDevices(devices);

    std::string configuration = serialPortConfigurationGenerator.CreateConfiguration(connectedDevices);

    std::string port_configuration_file("serial_port_cfg.json");

    return 0;
}
