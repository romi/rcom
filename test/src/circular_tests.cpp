#include <string>
#include "gtest/gtest.h"

extern "C" {
#include "mem.mock.h"
#include "mutex.mock.h"
}

#include "circular.h"


class circular_tests : public ::testing::Test
{
protected:
    circular_tests() = default;

	~circular_tests() override = default;

	void SetUp() override
    {
	    RESET_FAKE(safe_malloc);
        RESET_FAKE(safe_free);
	}

	void TearDown() override
    {
	}

};

TEST_F(circular_tests, new_circular_buffer_creates_buffer)
{
    // Arrange
    int size = 8;
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    // Act
    circular_buffer_t *cbuffer = new_circular_buffer(size);

    // Assert
    ASSERT_NE(cbuffer, nullptr);
    ASSERT_EQ(circular_buffer_size(cbuffer), size);
    delete_circular_buffer(cbuffer);
}

TEST_F(circular_tests, new_circular_buffer_ralloc_fails_returns_null)
{
    // Arrange
    int size = 8;
    void* r_1 = malloc(sizeof(circular_buffer_t));
    void* r_2 = nullptr;

    void* return_vals[2] = {r_1, r_2};//
    SET_RETURN_SEQ(safe_malloc, return_vals, 2)

    // Act
    circular_buffer_t *cbuffer = new_circular_buffer(size);

    // Assert
    ASSERT_EQ(cbuffer, nullptr);
    delete_circular_buffer(cbuffer);
    free(r_1);
}

TEST_F(circular_tests, delete_circular_buffer_deletes_data)
{
    // Arrange
    int size = 8;
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    // Act
    circular_buffer_t *cbuffer = new_circular_buffer(size);
    delete_circular_buffer(cbuffer);

    // Assert
    // 3 calls to safe_free in delete.
    ASSERT_EQ(safe_free_fake.call_count, 3 );
}

TEST_F(circular_tests, delete_circular_buffer_handles_null_pointer)
{
    // Arrange
    safe_free_fake.custom_fake = safe_free_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    circular_buffer_t *cbuffer = nullptr;

    // Act
    delete_circular_buffer(cbuffer);

    // Assert
    ASSERT_EQ(safe_free_fake.call_count, 0 );}

TEST_F(circular_tests, circular_buffer_availible_correct)
{
    // Arrange
    int size = 8;
    int used = 0;
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    // Act
    circular_buffer_t *cbuffer = new_circular_buffer(size);

    // Assert
    ASSERT_NE(cbuffer, nullptr);
    ASSERT_EQ(circular_buffer_size(cbuffer), size);
    ASSERT_EQ(circular_buffer_data_available(cbuffer), used);
    delete_circular_buffer(cbuffer);
}

TEST_F(circular_tests, circular_buffer_availible_correct_after_write)
{
    // Arrange
    int circular_buffer_size = 8;
    const int data_buffer_size = 3;

    int expected = data_buffer_size;
    const char data_buffer[data_buffer_size] = {0x01,0x02,0x03};

    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    circular_buffer_t *cbuffer = new_circular_buffer(circular_buffer_size);

    // Act
    circular_buffer_write(cbuffer, data_buffer, data_buffer_size);
    int actual = circular_buffer_data_available(cbuffer);
    // Assert
    ASSERT_EQ(actual, expected);
    delete_circular_buffer(cbuffer);
}

TEST_F(circular_tests, circular_buffer_space_correct)
{
    // Arrange
    int size = 8;
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    // Act
    circular_buffer_t *cbuffer = new_circular_buffer(size);

    // Assert
    ASSERT_NE(cbuffer, nullptr);
    ASSERT_EQ(circular_buffer_size(cbuffer), size);
    ASSERT_EQ(circular_buffer_space_available(cbuffer), size);
    delete_circular_buffer(cbuffer);
}

TEST_F(circular_tests, circular_buffer_space_correct_after_write)
{
    // Arrange
    int circular_buffer_size = 8;
    const int data_buffer_size = 3;

    int expected = circular_buffer_size - data_buffer_size;
    const char data_buffer[data_buffer_size] = {0x01,0x02,0x03};

    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    circular_buffer_t *cbuffer = new_circular_buffer(circular_buffer_size);

    // Act
    circular_buffer_write(cbuffer, data_buffer, data_buffer_size);
    int actual = circular_buffer_space_available(cbuffer);
    // Assert
    ASSERT_EQ(actual, expected);
    delete_circular_buffer(cbuffer);
}

TEST_F(circular_tests, circular_buffer_availible_correct_after_over_write)
{
    // Arrange
    int circular_buffer_size = 8;
    const int data_buffer_size = 4;
    int expected = data_buffer_size;
 //   const char data_buffer[data_buffer_size] = {0x01,0x02,0x03,0x04, 0x05};
    const char data_buffer[data_buffer_size] = {0x01,0x02,0x03,0x04};

    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    circular_buffer_t *cbuffer = new_circular_buffer(circular_buffer_size);

    // Act
    circular_buffer_write(cbuffer, data_buffer, data_buffer_size);
    int actual = circular_buffer_data_available(cbuffer);
    circular_buffer_write(cbuffer, data_buffer, data_buffer_size);
    circular_buffer_write(cbuffer, data_buffer, data_buffer_size);
    actual = circular_buffer_data_available(cbuffer);

    // Assert
    ASSERT_EQ(actual, expected);
    delete_circular_buffer(cbuffer);
}

TEST_F(circular_tests, circular_buffer_availible_correct_after_read_over_write)
{
    // Arrange
    int circular_buffer_size = 8;
    const int data_buffer_size = 4;
    const int read_data_buffer_size = 32;
    int expected = data_buffer_size;
    //   const char data_buffer[data_buffer_size] = {0x01,0x02,0x03,0x04, 0x05};
    const char data_buffer[data_buffer_size] = {0x01,0x02,0x03,0x04};
    char read_data_buffer[read_data_buffer_size];

    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    circular_buffer_t *cbuffer = new_circular_buffer(circular_buffer_size);

    // Act
    circular_buffer_write(cbuffer, data_buffer, data_buffer_size);
    int actual = circular_buffer_data_available(cbuffer);
    circular_buffer_write(cbuffer, data_buffer, data_buffer_size);
    circular_buffer_read(cbuffer, read_data_buffer, read_data_buffer_size);
    circular_buffer_write(cbuffer, data_buffer, data_buffer_size);
    actual = circular_buffer_data_available(cbuffer);

    // Assert
    ASSERT_EQ(actual, expected);
    delete_circular_buffer(cbuffer);
}


TEST_F(circular_tests, circular_buffer_space_correct_after_read_over_write)
{
    // Arrange
    int circular_buffer_size = 8;
    const int data_buffer_size = 4;
    const int read_data_buffer_size = 32;
    int expected = data_buffer_size;
    const char data_buffer[data_buffer_size] = {0x01,0x02,0x03,0x04};
    char read_data_buffer[read_data_buffer_size];

    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    circular_buffer_t *cbuffer = new_circular_buffer(circular_buffer_size);

    // Act
    circular_buffer_write(cbuffer, data_buffer, data_buffer_size);
    circular_buffer_write(cbuffer, data_buffer, data_buffer_size);
    circular_buffer_read(cbuffer, read_data_buffer, read_data_buffer_size);
    circular_buffer_write(cbuffer, data_buffer, data_buffer_size);
    int actual = circular_buffer_space_available(cbuffer);

    // Assert
    ASSERT_EQ(actual, expected);
    delete_circular_buffer(cbuffer);
}

TEST_F(circular_tests, circular_buffer_write_single_over_write_rejected)
{
    // Arrange
    int circular_buffer_size = 8;
    const int data_buffer_size = 9;
    const char data_buffer[data_buffer_size] = {0x01,0x02,0x03,0x04, 0x05,0x06,0x07,0x08, 0x09};

    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    circular_buffer_t *cbuffer = new_circular_buffer(circular_buffer_size);

    int expected_read = cbuffer->readpos;
    int expected_write = cbuffer->writepos;
    int expected_availible = circular_buffer_data_available(cbuffer);

    // Act
    circular_buffer_write(cbuffer, data_buffer, data_buffer_size);

    int actual_read = cbuffer->readpos;
    int actual_write = cbuffer->writepos;
    int actual_available = circular_buffer_data_available(cbuffer);
    // Assert
    ASSERT_EQ(actual_read, expected_read);
    ASSERT_EQ(actual_write, expected_write);
    ASSERT_EQ(actual_available, expected_availible);

    delete_circular_buffer(cbuffer);
}

TEST_F(circular_tests, circular_buffer_read_too_much_reads_buffer_size)
{
    // Arrange
    int circular_buffer_size = 8;
    const int data_buffer_size = 8;
    const char data_buffer[data_buffer_size] = {0x01,0x02,0x03,0x04, 0x05,0x06,0x07,0x08};
    const int read_data_buffer_size = 9;
    char read_data_buffer[data_buffer_size];

    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    circular_buffer_t *cbuffer = new_circular_buffer(circular_buffer_size);

    int expected_available = circular_buffer_data_available(cbuffer);

    // Act
    circular_buffer_write(cbuffer, data_buffer, 1);
    circular_buffer_read(cbuffer, read_data_buffer, 1);
    circular_buffer_write(cbuffer, data_buffer, data_buffer_size);
    circular_buffer_read(cbuffer, read_data_buffer, read_data_buffer_size);

    int actual_available = circular_buffer_data_available(cbuffer);
    // Assert
    ASSERT_EQ(actual_available, expected_available);

    delete_circular_buffer(cbuffer);
}
