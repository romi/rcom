#ifndef RCDISCOVER_SERIALPORTDISCOVER_H
#define RCDISCOVER_SERIALPORTDISCOVER_H

#include <iostream>
#include <vector>
#include "ISerialPortDiscover.h"

class SerialPortDiscover : public ISerialPortDiscover {
    public:
        SerialPortDiscover() = default;
        virtual ~SerialPortDiscover() = default;
        std::string ConnectedDevice(const std::string& path);
};

#endif //RCDISCOVER_SERIALPORTDISCOVER_H
