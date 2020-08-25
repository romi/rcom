#include <experimental/filesystem>
#include <fstream>
#include <string>
#include <future>

#include <r.h>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "SerialPortConfigurationGenerator.h"

namespace fs = std::experimental::filesystem;
using testing::UnorderedElementsAre;
using testing::Return;

class SerialPortConfigurationGenerator_tests : public ::testing::Test
{
protected:
	SerialPortConfigurationGenerator_tests() = default;

	~SerialPortConfigurationGenerator_tests() override = default;

	void SetUp() override
    {
	}

	void TearDown() override
    {
	}

    std::string ReadFileAsString(const std::string& filePath)
    {
        std::ostringstream contents;
        try
        {
            std::ifstream in;
            in.exceptions(std::ifstream::badbit | std::ifstream::failbit);
            in.open(filePath);
            contents << in.rdbuf();
        }
        catch (const std::exception& ex)
        {
            std::cout << ex.what();
            throw(ex);
        }

        return(contents.str());
    }

protected:

};

TEST_F(SerialPortConfigurationGenerator_tests, SerialPortConfigurationGenerator_can_construct)
{
    // Arrange
    // Act
    //Assert
    ASSERT_NO_THROW(SerialPortConfigurationGenerator SerialPortConfigurationGenerator);
}


TEST_F(SerialPortConfigurationGenerator_tests, SerialPortConfigurationGenerator_when_no_devices_present_creates_skeleton)
{
    // Arrange
    std::vector<std::pair<std::string, std::string>> attached_devices;
    SerialPortConfigurationGenerator SerialPortConfigurationGenerator;
    std::string expected_json("{\"ports\":{}}"
    );

    // Act
    auto actual = SerialPortConfigurationGenerator.CreateConfiguration(attached_devices);

    //Assert
    ASSERT_EQ(actual, expected_json);
}

TEST_F(SerialPortConfigurationGenerator_tests, SerialPortConfigurationGenerator_when_devices_present_creates_config)
{
    // Arrange
    std::vector<std::pair<std::string, std::string>> attached_devices;
    SerialPortConfigurationGenerator SerialPortConfigurationGenerator;

    attached_devices.push_back(std::make_pair(std::string("/dev/ACM0"), std::string("brushmotorcontroller")));
    attached_devices.push_back(std::make_pair(std::string("/dev/ACM1"), std::string("cnc")));
    std::string expected_json("{\"ports\":{\"cnc\":{\"port\":\"/dev/ACM1\",\"type\":\"serial\"},\"brushmotorcontroller\":{\"port\":\"/dev/ACM0\",\"type\":\"serial\"}}}");

    // Act
    auto actual = SerialPortConfigurationGenerator.CreateConfiguration(attached_devices);

    //Assert
    ASSERT_EQ(actual, expected_json);
}

TEST_F(SerialPortConfigurationGenerator_tests, SerialPortConfigurationGenerator_save_configuration_fails_when_cant_write)
{
    // Arrange
    SerialPortConfigurationGenerator SerialPortConfigurationGenerator;
    std::string file_contents("The quick brown fox jumped over the lazy dog.");
    std::string file_name("/impossible/serial_config.json");
    // Act
    auto actual = SerialPortConfigurationGenerator.SaveConfiguration(file_name, file_contents);

    //Assert
    ASSERT_FALSE(actual);
}

TEST_F(SerialPortConfigurationGenerator_tests, SerialPortConfigurationGenerator_save_configuration_succeeds_when_written)
{
    // Arrange
    SerialPortConfigurationGenerator SerialPortConfigurationGenerator;
    std::string expected_file_contents("The quick brown fox jumped over the lazy dog.");
    std::string file_name("serial_config.json");
    std::string actual_file_contents;

    // Act
    auto actual = SerialPortConfigurationGenerator.SaveConfiguration(file_name, expected_file_contents);

    //Assert
    ASSERT_NO_THROW(actual_file_contents = ReadFileAsString(file_name));
    ASSERT_EQ(actual_file_contents, expected_file_contents);
    ASSERT_TRUE(actual);
}

