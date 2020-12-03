#include <string>
#include "gtest/gtest.h"

#include "net.h"

extern "C" {
#include "log.mock.h"
#include "socket.mock.h"
}

class net_tests : public ::testing::Test
{
protected:
    net_tests() = default;

    ~net_tests() override = default;

    void SetUp() override
    {
        RESET_FAKE(socket());
    }

    void TearDown() override
    {
    }
};


TEST_F(net_tests, socket_returns_false)
{
    // Arrange
    // Act


    //Assert
}
