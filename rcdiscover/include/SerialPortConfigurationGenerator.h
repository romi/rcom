#ifndef ROMI_ROVER_BUILD_AND_TEST_SERIALPORTCONFIGURATIONGENERATOR_H
#define ROMI_ROVER_BUILD_AND_TEST_SERIALPORTCONFIGURATIONGENERATOR_H

#include <string>
#include <vector>
#include "ISerialPortConfigurationGenerator.h"


class SerialPortConfigurationGenerator : public ISerialPortConfigurationGenerator
{
public:
        SerialPortConfigurationGenerator();
        virtual ~SerialPortConfigurationGenerator() = default;
        std::string CreateConfiguration(std::vector<std::pair<std::string, std::string>>& devices) override;
        bool SaveConfiguration(const std::string& configuration_file, const std::string& configuration_json) override;
private:
        const std::string serial_port_configuration_key;
        const std::string serial_port_key;
        const std::string serial_device_key;
};


#endif //ROMI_ROVER_BUILD_AND_TEST_SERIALPORTCONFIGURATIONGENERATOR_H
