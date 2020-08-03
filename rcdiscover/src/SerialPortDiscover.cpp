#include <r.h>
#include "SerialPortDiscover.h"

//std::string
//SerialPortDiscover::ConnectedDevice(const std::string& path)
//{
//    std::string device;
//    std::string command = "?\r\n";
//    int32 timeout_ms = 500;
//    const int bufflen = 256;
//    char buffer[bufflen];
//    memset(buffer, 0, bufflen);
//    std::cout << "opening: " << path.c_str() << "timeout" << timeout_ms << std::endl;
//    serial_t * serial_port = new_serial(path.c_str(), 115200, 0);
//    if ( serial_port )
//    {
//        if (serial_write(serial_port, command.c_str(), command.length()) == 0)
//        {
//            membuf_t membuf;
//            const char *reply = serial_readline(serial_port, &membuf);
// //           if (serial_read_timeout(serial_port, buffer, bufflen, timeout_ms) == 0)
//            {
//                device = reply;
//            }
//        }
//    }
//    delete_serial(serial_port);
//    return device;
//}

std::string
SerialPortDiscover::ConnectedDevice(const std::string& path)
{
    std::string device;
    std::string command = "?";
    int32 timeout_ms = 200;
    const int bufflen = 256;
    char buffer[bufflen];
    memset(buffer, 0, bufflen);
    std::cout << "opening: " << std::endl;
    serial_t * serial_port = new_serial(path.c_str(), 115200, 0);
    if ( serial_port )
    {
        std::cout << "ConnectedDevice: writing" << command.c_str() << std::endl;
        if  (serial_println(serial_port, command.c_str()) == 0)
        {
            std::cout << "ConnectedDevice: reading" << std::endl;
            if (serial_read_timeout(serial_port, buffer, bufflen, timeout_ms) == 0)
            {
                device = buffer;
            }
        }
    }
    delete_serial(serial_port);
    return device;
}