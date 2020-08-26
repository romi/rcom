#include <iostream>
#include <vector>
#include <fstream>
#include <experimental/filesystem>

#include <r.h>
#include "SerialPortConfigurationGenerator.h"

SerialPortConfigurationGenerator::SerialPortConfigurationGenerator()
:
        serial_ports_configuration_key("ports"), serial_port_key("port"), serial_device_type("type"), serial_type("serial")
{

}

json_object_t SerialPortConfigurationGenerator::CreateConfigurationBase(const std::string& json_configuration)
{
    json_object_t configuration_object;
    if (json_configuration.empty())
    {
        configuration_object = json_object_create();
    } else
    {
        int parse_error = 0;
        char error_message[256];
        configuration_object = json_parse_ext(json_configuration.c_str(), &parse_error, error_message, sizeof(error_message));
        if (json_isnull(configuration_object))
        {
            configuration_object = json_object_create();
        }
    }
    return configuration_object;
}

std::string SerialPortConfigurationGenerator::CreateConfiguration(const std::string& json_configuration, const std::vector<std::pair<std::string, std::string>>& devices)
{
    const int buff_size = 2048;
    char json_string_buff[buff_size];
    memset(json_string_buff, 0, buff_size);
    std::string json_string;

    json_object_t configuration_object = CreateConfigurationBase(json_configuration);
    json_object_t ports_object = json_object_create();
    json_object_set(configuration_object, serial_ports_configuration_key.c_str(), ports_object);

    for (const auto& device : devices)
    {
        json_object_t device_object = json_object_create();
        json_object_setstr(device_object, serial_device_type.c_str(), serial_type.c_str());
        json_object_setstr(device_object, serial_port_key.c_str(), device.first.c_str());
        json_object_set(ports_object, device.second.c_str(), device_object);
        json_unref(device_object);
    }

    json_tostring(configuration_object, json_string_buff, buff_size);
    json_string = json_string_buff;

    json_unref(configuration_object);
    json_unref(ports_object);

    return json_string;
}

std::string SerialPortConfigurationGenerator::LoadConfiguration(const std::string& configuration_file) const
{
    std::ostringstream contents;
    try
    {
        std::ifstream in;
        in.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        in.open(configuration_file);
        contents << in.rdbuf();
    }
    catch (const std::exception& ex)
    {
        std::cout << __PRETTY_FUNCTION__  << " failed to load file: '" << configuration_file << "' exception: " << ex.what() << std::endl;
    }

    return(contents.str());
}

bool SerialPortConfigurationGenerator::SaveConfiguration(const std::string& configuration_file, const std::string& configuration_json)
{
    bool result = true;
    try
    {
        std::ofstream out;
        out.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        out.open(configuration_file);
        out << configuration_json;
        std::cout << "wrote config: " << configuration_file << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cout << __PRETTY_FUNCTION__  << " failed to write file: '" << configuration_file << "' exception: " << ex.what() << std::endl;
        result = false;
    }
    return result;
}