#include <r.h>
#include <iostream>
#include "SerialPortDiscover.h"

SerialPortDiscover::SerialPortDiscover(const std::map<std::string, std::string>& deviceFilter) : knownDevicesMap(deviceFilter)
{
}

std::string
SerialPortDiscover::ConnectedDevice(const std::string& path, const int32_t timeout_ms)
{
    std::string device;
    std::string command = "?";
    const int bufflen = 256;
    char buffer[bufflen];
    memset(buffer, 0, bufflen);
    serial_t * serial_port = new_serial(path.c_str(), 115200, 0);
    if ( serial_port )
    {
        if  (serial_println(serial_port, command.c_str()) == 0)
        {
            if (serial_read_timeout(serial_port, buffer, bufflen, timeout_ms) == 0)
            {
                std::cout << path << " " << buffer << std::endl;
                device = FilterDevice(buffer);
            }
        }
    }
    delete_serial(serial_port);
    return device;
}

std::string
SerialPortDiscover::FilterDevice(char* buffer)
{
    std::string buffer_string(buffer);
    for (const auto& device : knownDevicesMap)
    {
            if (buffer_string.find(device.first) != std::string::npos) {
                std::cout << "Found: " << device.second << std::endl;
                return device.second;
            }
    }
    return std::string();
}
