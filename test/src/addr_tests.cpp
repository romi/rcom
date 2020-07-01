#include <string>
#include "gtest/gtest.h"

extern "C" {
#include "fff.h"
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
	}

	void TearDown() override
    {
	}

};

TEST_F(addr_tests, new_addr_when_r_new_fails_logs_returns_NULL)
{
    // Arrange

    // Act
    addr_t *paddr = new_addr0();

    //Assert
    ASSERT_NE(paddr, nullptr);
    delete_addr(paddr);

}