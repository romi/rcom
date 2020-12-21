#include <string>

#include "gtest/gtest.h"
#include "addr.h"
#include "MessageLink.h"
#include "RegistryEntry.h"


class RegistryEntry_tests : public ::testing::Test
{
protected:
    RegistryEntry_tests() = default;

    ~RegistryEntry_tests() override = default;

    void SetUp() override
    {
    }

    void TearDown() override
    {

    }

};

TEST_F(RegistryEntry_tests, can_construct)
{
    // Arrange
    addr_t address;
    addr_set_ip(&address, "127.0.0.1");
    std::shared_ptr<ILink> Link = std::make_shared<MessageLink>();
    // Act
    //Assert
    ASSERT_NO_THROW(RegistryEntry entry("name", "topic", RegistryType::TYPE_MESSAGELINK, address, Link));
}

TEST_F(RegistryEntry_tests, can_copy_construct)
{
    // Arrange
    addr_t address;
    addr_set_ip(&address, "127.0.0.1");
    std::shared_ptr<ILink> Link = std::make_shared<MessageLink>();
    // Act
    //Assert
    RegistryEntry entry("name", "topic", RegistryType::TYPE_MESSAGELINK, address, Link);
    RegistryEntry entry_copy(entry);

}

TEST_F(RegistryEntry_tests, address_correct_copy_construct)
{
    // Arrange
    std::string expected("127.0.0.1");
    addr_t address;
    addr_set_ip(&address, expected.c_str());
    std::shared_ptr<ILink> Link = std::make_shared<MessageLink>();
    // Act
    //Assert
    RegistryEntry entry("name", "topic", RegistryType::TYPE_MESSAGELINK, address, Link);
    RegistryEntry entry_copy(entry);

    auto actual = entry_copy.address();
    ASSERT_EQ(expected, std::string(inet_ntoa(actual.sin_addr)));
}
