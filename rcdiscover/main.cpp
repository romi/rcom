#include <iostream>
#include <vector>
#include <experimental/filesystem>
#include "SerialPortIdentification.h"
#include "SerialPortDiscover.h"

namespace fs = std::experimental::filesystem;

int main() {
    std::string dev = "/dev";
    SerialPortDiscover serialPortDiscover;
    SerialPortIdentification serialPortIdentification(serialPortDiscover);

    auto devices = serialPortIdentification.ListFilesOfType(dev, std::string("ACM"));

    auto deviceNames = serialPortIdentification.ConnectedDevices(devices);

    for (auto device : deviceNames)
    {
        std::cout << "port " << device.first << std::endl;
        std::cout << "device " << device.second << std::endl;
    }

    return 0;
}
