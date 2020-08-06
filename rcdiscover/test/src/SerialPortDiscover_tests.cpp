#include <experimental/filesystem>
#include <fstream>
#include <string>
#include <future>

#include <r.h>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "SerialPortDiscover.h"
#include "TestUtil.hpp"

namespace fs = std::experimental::filesystem;
using testing::UnorderedElementsAre;
using testing::Return;

class SerialPortDiscover_tests : public ::testing::Test
{
protected:
	SerialPortDiscover_tests() = default;

	~SerialPortDiscover_tests() override = default;

	void SetUp() override
    {
	}

	void TearDown() override
    {
	}

	void CreateFiles()
    {
    }

    void DeleteFiles()
    {
    }

protected:

};

TEST_F(SerialPortDiscover_tests, SerialPortDiscover_can_construct)
{
    // Arrange
    // Act
    //Assert
    ASSERT_NO_THROW(SerialPortDiscover SerialPortDiscover);
}

TEST_F(SerialPortDiscover_tests, SerialPortDiscover_ConnectedDevice_bad_port_returns_empty_string)
{
    // Arrange
    SerialPortDiscover SerialPortDiscover;
    const int32_t timeout_ms = 100;
    // Act
    auto device = SerialPortDiscover.ConnectedDevice(std::string("/dev/notreal"), timeout_ms);

    //Assert
    ASSERT_TRUE(device.empty());
}

TEST_F(SerialPortDiscover_tests, SerialPortDiscover_ConnectedDevice_times_out_returns_empty_string)
{
    // Arrange
    SerialPortDiscover SerialPortDiscover;
    std::string port0 = CppLinuxSerial::TestUtil::GetInstance().GetDevice0Name();
    const int32_t timeout_ms = 100;
    // Act
    auto device = SerialPortDiscover.ConnectedDevice(port0, timeout_ms);

    //Assert
    ASSERT_TRUE(device.empty());
}

int ThreadFunction(const std::string& port, const std::string& controller)
{
    const int bufflen = 256;
    int result = -1;
    char buffer[bufflen];
    memset(buffer, 0, bufflen);

    serial_t * serial_port = new_serial(port.c_str(), 115200, 0);
    if ( serial_port )
    {
        if (serial_read_timeout(serial_port, buffer, bufflen, 1000) == 0)
        {
            if (serial_write(serial_port, controller.c_str(), controller.length()) == 0)
            {
                result = 0;
            }
        }
    }
    else
    {
        std::cout << "ThreadFunction failed to open " << port << std::endl;
    }
    delete_serial(serial_port);
    return result;
}

TEST_F(SerialPortDiscover_tests, SerialPortDiscover_ConnectedDevice_returns_connected_device)
{
    // Arrange
    std::string controller("brushmotorcontroller\n");

    std::string port0 = CppLinuxSerial::TestUtil::GetInstance().GetDevice0Name();
    std::string port1 = CppLinuxSerial::TestUtil::GetInstance().GetDevice1Name();

    auto future = std::async(ThreadFunction, port1, controller);
    SerialPortDiscover SerialPortDiscover;
    const int timeout_ms = 500;
    // Act
    auto device = SerialPortDiscover.ConnectedDevice(port0, timeout_ms);

    // Assert
    ASSERT_EQ(controller, device);

}