#include <string>

#include "gtest/gtest.h"
#include "UuId.h"


class UuId_tests : public ::testing::Test
{
protected:
    UuId_tests() = default;

    ~UuId_tests() override = default;

    void SetUp() override
    {
    }

    void TearDown() override
    {

    }

};

TEST_F(UuId_tests, ruuid_returns_correct_length_string)
{
    // Arrange
    int actualsize = (16*2) + 4; // 4 '-' chars.
    // Act
    std::string actual = UuId::create();

    // Assert
    ASSERT_EQ(actual.size(), actualsize);
}