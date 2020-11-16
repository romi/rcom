#include <string>
#include "gtest/gtest.h"

extern "C" {
#include "mem.mock.h"
}

// Need to include r.h here to make sure when we mock them below they are mocked in the link with the tests.
#include "r.h"
#include "addr.h"

FAKE_VOID_FUNC_VARARG(r_err, const char*, ...)

class addr_tests : public ::testing::Test
{
protected:
	addr_tests() = default;

	~addr_tests() override = default;

	void SetUp() override
    {
        RESET_FAKE(r_err);
        safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
        safe_free_fake.custom_fake = safe_free_custom_fake;
	}

	void TearDown() override
    {
	}

};

TEST_F(addr_tests, new_addr0_creates_0_ip)
{
    // Arrange

    // Act
    addr_t *paddr = new_addr0();

    //Assert
    ASSERT_NE(paddr, nullptr);
    ASSERT_EQ(paddr->sin_family, AF_INET);
    ASSERT_EQ(paddr->sin_port, 0);
    ASSERT_EQ(paddr->sin_addr.s_addr, 0);
    ASSERT_EQ(paddr->sin_zero[0], 0);
    delete_addr(paddr);

}

TEST_F(addr_tests, new_addr_null_IP_creates_0_ip)
{
    // Arrange
    int port = 2;

    // Act
    addr_t *paddr = new_addr(nullptr, port);

    //Assert
    ASSERT_NE(paddr, nullptr);
    ASSERT_EQ(paddr->sin_family, AF_INET);
    ASSERT_EQ(paddr->sin_port, htons(port));
    ASSERT_EQ(paddr->sin_addr.s_addr, 0);
    ASSERT_EQ(paddr->sin_zero[0], 0);
    delete_addr(paddr);
}

TEST_F(addr_tests, new_addr_too_short_returns_NULL)
{
    // Arrange
    int port = 2;
    const char *address = "0.0.0.";

    // Act
    addr_t *paddr = new_addr(address, port);

    //Assert
    ASSERT_EQ(paddr, nullptr);
    delete_addr(paddr);
}

TEST_F(addr_tests, new_addr_too_long_returns_NULL)
{
    // Arrange
    int port = 2;
    const char *address = "1234.000.000.000";

    // Act
    addr_t *paddr = new_addr(address, port);

    //Assert
    ASSERT_EQ(paddr, nullptr);
    delete_addr(paddr);
}

TEST_F(addr_tests, new_addr_port_too_low_returns_NULL)
{
    // Arrange
    int port = -1;
    const char *address = "127.0.0.1";

    // Act
    addr_t *paddr = new_addr(address, port);

    //Assert
    ASSERT_EQ(paddr, nullptr);
    delete_addr(paddr);
}

TEST_F(addr_tests, new_addr_port_too_high_returns_NULL)
{
    // Arrange
    int port = 65537;
    const char *address = "127.0.0.1";

    // Act
    addr_t *paddr = new_addr(address, port);

    //Assert
    ASSERT_EQ(paddr, nullptr);
    delete_addr(paddr);
}

TEST_F(addr_tests, new_addr_returns_new_addr)
{
    // Arrange
    int port = 65535;
    const char *address = "127.0.0.1";
    const int binary_address = 0x0100007f; //01 00 00 127
    // Act
    addr_t *paddr = new_addr(address, port);

    //Assert
    ASSERT_NE(paddr, nullptr);
    ASSERT_EQ(paddr->sin_family, AF_INET);
    ASSERT_EQ(paddr->sin_port, htons(port));
    ASSERT_EQ(paddr->sin_addr.s_addr, binary_address);
    delete_addr(paddr);
}

TEST_F(addr_tests, addr_clone_null_returns_null)
{
    // Arrange
    // Act
    addr_t *paddr = addr_clone(nullptr);

    //Assert
    ASSERT_EQ(paddr, nullptr);
    delete_addr(paddr);
}

TEST_F(addr_tests, addr_clone_returns_clone)
{
    // Arrange
    int port = 65535;
    const char *address = "127.0.0.1";
    addr_t *paddr = new_addr(address, port);

    // Act
    addr_t *paddr_clone = addr_clone(paddr);

    //Assert
    ASSERT_NE(paddr_clone, nullptr);
    ASSERT_EQ(paddr->sin_family, paddr_clone->sin_family);
    ASSERT_EQ(paddr->sin_port, paddr_clone->sin_port);
    ASSERT_EQ(paddr->sin_addr.s_addr, paddr_clone->sin_addr.s_addr);
    delete_addr(paddr);
    delete_addr(paddr_clone);
}

TEST_F(addr_tests, addr_copy_copies_data)
{
    // Arrange
    int port = 65535;
    const char *address = "127.0.0.1";
    addr_t *paddr = new_addr(address, port);
    addr_t *paddr_copy = new_addr0();

    // Act
    addr_copy(paddr, paddr_copy);

    //Assert
    ASSERT_EQ(paddr->sin_family, paddr_copy->sin_family);
    ASSERT_EQ(paddr->sin_port, paddr_copy->sin_port);
    ASSERT_EQ(paddr->sin_addr.s_addr, paddr_copy->sin_addr.s_addr);
    delete_addr(paddr);
    delete_addr(paddr_copy);
}


TEST_F(addr_tests, addr_port_correct)
{
    // Arrange
    int port = 100;
    const char *address = "127.0.0.1";

    // Act
    addr_t *paddr = new_addr(address, port);
    int actual = addr_port(paddr);

    //Assert

    ASSERT_EQ(actual, port);
    delete_addr(paddr);
}

TEST_F(addr_tests, addr_ip_correct)
{
    // Arrange
    int port = 100;
    const char *address = "127.0.0.1";

    const int len = 32;
    char actual[len];

    // Act
    addr_t *paddr = new_addr(address, port);
    addr_ip(paddr, actual, len);

    //Assert

    ASSERT_EQ(std::string(actual), std::string(address));
    delete_addr(paddr);
}

TEST_F(addr_tests, addr_ip_truncated)
{
    // Arrange
    int port = 100;
    std::string address("127.0.0.1");

    const int len = 6;
    char actual[len];

    std::string expected = address.substr(0, len-1); // -1 for null terminator.

    // Act
    addr_t *paddr = new_addr(address.c_str(), port);
    addr_ip(paddr, actual, len);

    //Assert

    ASSERT_EQ(std::string(actual), expected);
    delete_addr(paddr);
}

TEST_F(addr_tests, addr_string_null_returns_null_string)
{
    // Arrange
    const int len = 16;
    char buff[len];

    std::string expected("(null)");

    // Act
    addr_string(nullptr, buff, len);
    //Assert

    ASSERT_EQ(std::string(buff), expected);
}

TEST_F(addr_tests, addr_string_addr_returns_addr_string)
{
    // Arrange
    int port = 100;
    const char *address = "127.0.0.1";
    const int len = 32;
    char actual[len];
    std::string expected (std::string(address) + ":" + std::to_string(port));

    // Act
    addr_t *paddr = new_addr(address, port);
    addr_string(paddr, actual, len);

    //Assert

    ASSERT_EQ(std::string(actual), expected);
}

TEST_F(addr_tests, addr_eq_ports_differ_fails)
{
    // Arrange
    int port1 = 100;
    int port2 = 101;
    std::string address("127.0.0.1");

    // Act
    addr_t *address_1 = new_addr(address.c_str(), port1);
    addr_t *address_2 = new_addr(address.c_str(), port2);
    int actual = addr_eq(address_1, address_2);

    //Assert

    ASSERT_EQ(actual, 0);
    delete_addr(address_1);
    delete_addr(address_2);
}

TEST_F(addr_tests, addr_eq_address_differ_fails)
{
    // Arrange
    int port1 = 100;
    std::string address1("127.0.0.1");
    std::string address2("127.0.0.2");

    // Act
    addr_t *address_1 = new_addr(address1.c_str(), port1);
    addr_t *address_2 = new_addr(address2.c_str(), port1);
    int actual = addr_eq(address_1, address_2);

    //Assert

    ASSERT_EQ(actual, 0);
    delete_addr(address_1);
    delete_addr(address_2);
}

TEST_F(addr_tests, addr_eq_address_differ_reversed_fails)
{
    // Arrange
    int port1 = 100;
    std::string address1("127.0.0.1");
    std::string address2("127.0.0.2");

    // Act
    addr_t *address_1 = new_addr(address1.c_str(), port1);
    addr_t *address_2 = new_addr(address2.c_str(), port1);
    int actual = addr_eq(address_2, address_1);

    //Assert

    ASSERT_EQ(actual, 0);
    delete_addr(address_1);
    delete_addr(address_2);
}

TEST_F(addr_tests, addr_eq_address_same_succeeds)
{
    // Arrange
    int port1 = 100;
    std::string address1("127.0.0.1");
    std::string address2("127.0.0.1");

    // Act
    addr_t *address_1 = new_addr(address1.c_str(), port1);
    addr_t *address_2 = new_addr(address2.c_str(), port1);
    int actual = addr_eq(address_2, address_1);

    //Assert

    ASSERT_EQ(actual, 1);
    delete_addr(address_1);
    delete_addr(address_2);
}

TEST_F(addr_tests, addr_parse_ip_only_fails)
{
    // Arrange
    std::string address_string("127.0.0.1");

    // Act
    addr_t *p_address = addr_parse(address_string.c_str());

    //Assert

    ASSERT_EQ(p_address, nullptr);
    delete_addr(p_address);
}

TEST_F(addr_tests, addr_parse_invalid_ip_fails)
{
    // Arrange
    std::string address_string("127.0.0.xx:100");

    // Act
    addr_t *p_address = addr_parse(address_string.c_str());

    //Assert

    ASSERT_EQ(p_address, nullptr);
    delete_addr(p_address);
}


TEST_F(addr_tests, addr_parse_invalid_port_fails)
{
    // Arrange
    std::string address_string("127.0.0.1:xx");

    // Act
    addr_t *p_address = addr_parse(address_string.c_str());

    //Assert

    ASSERT_EQ(p_address, nullptr);
    delete_addr(p_address);
}

TEST_F(addr_tests, addr_parse_invalid_port_string_fails)
{
    // Arrange
    std::string address_string("127.0.0.1:100xx");

    // Act
    addr_t *p_address = addr_parse(address_string.c_str());

    //Assert

    ASSERT_EQ(p_address, nullptr);
    delete_addr(p_address);
}

TEST_F(addr_tests, addr_parse_invalid_port_number_fails)
{
    // Arrange
    std::string address_string("127.0.0.1:70000");

    // Act
    addr_t *p_address = addr_parse(address_string.c_str());

    //Assert

    ASSERT_EQ(p_address, nullptr);
    delete_addr(p_address);
}

TEST_F(addr_tests, addr_parse_valid_string_succeeds)
{
    // Arrange
    std::string address_string("127.0.0.1:100");

    // Act
    addr_t *p_address = addr_parse(address_string.c_str());

    //Assert

    ASSERT_NE(p_address, nullptr);
    delete_addr(p_address);
}
