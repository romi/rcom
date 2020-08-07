#include <experimental/filesystem>
#include <fstream>
#include <string>
#include <future>
#include <thread>

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
    std::vector<std::string> knownDevices = {"Grbl", "BrushMotorController", "RomiControlPanel"};

};

TEST_F(SerialPortDiscover_tests, SerialPortDiscover_can_construct)
{
    // Arrange
    // Act
    //Assert
    ASSERT_NO_THROW(SerialPortDiscover SerialPortDiscover(knownDevices));
}

TEST_F(SerialPortDiscover_tests, SerialPortDiscover_ConnectedDevice_bad_port_returns_empty_string)
{
    // Arrange
    SerialPortDiscover SerialPortDiscover(knownDevices);
    const int32_t timeout_ms = 100;
    // Act
    auto device = SerialPortDiscover.ConnectedDevice(std::string("/dev/notreal"), timeout_ms);

    //Assert
    ASSERT_TRUE(device.empty());
}

TEST_F(SerialPortDiscover_tests, SerialPortDiscover_ConnectedDevice_times_out_returns_empty_string)
{
    // Arrange
    SerialPortDiscover SerialPortDiscover(knownDevices);
    std::string port0 = CppLinuxSerial::TestUtil::GetInstance().GetDevice0Name();
    const int32_t timeout_ms = 100;
    // Act
    auto device = SerialPortDiscover.ConnectedDevice(port0, timeout_ms);

    //Assert
    ASSERT_TRUE(device.empty());
}

int ThreadFunction(const std::string& port, const std::string& controller, std::condition_variable *cv)
{

    const int bufflen = 256;
    int result = -1;
    char buffer[bufflen];
    memset(buffer, 0, bufflen);
    serial_t * serial_port = new_serial(port.c_str(), 115200, 0);
    cv->notify_one();
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
    const int timeout_ms = 500;
    std::mutex mutex;
    std::unique_lock<std::mutex> lk(mutex);
    std::condition_variable cv;
    std::string actual_device;
    std::string expected_device(knownDevices[0]);
    std::string port0 = CppLinuxSerial::TestUtil::GetInstance().GetDevice0Name();
    std::string port1 = CppLinuxSerial::TestUtil::GetInstance().GetDevice1Name();

    SerialPortDiscover SerialPortDiscover(knownDevices);
    // NOTE: DON'T FORGET \n its canonical!
    auto future = std::async(ThreadFunction, port1, expected_device+"\n", &cv);

    // Act
    if (cv.wait_for(lk, std::chrono::milliseconds(timeout_ms)) != std::cv_status::timeout)
    {
        actual_device = SerialPortDiscover.ConnectedDevice(port0, timeout_ms);
    }
    int thread_success = future.get();

    // Assert
    ASSERT_EQ(expected_device, actual_device);
    ASSERT_EQ(thread_success, 0);
}