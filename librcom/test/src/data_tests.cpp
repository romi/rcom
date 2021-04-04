#include <string>
#include <gmock/gmock-matchers.h>
#include "gtest/gtest.h"

extern "C" {
#include "mem.mock.h"
#include "log.mock.h"
#include "json.mock.h"
#include "clock_posix.mock.h"
}
#include "data.h"

// Using a lamnda to set a fake function.
//    json_tostring_fake.custom_fake = [](json_object_t object __attribute((unused)), char* buffer, int32 bufflen)->int32
//    {
//        std::string expected_json_string("{\"key\":\"value\"}");
//        strncpy(buffer, expected_json_string.c_str(), bufflen);
//        return 0;
//    };



class data_tests : public ::testing::Test
{
protected:
    data_tests() = default;

	~data_tests() override = default;

	void SetUp() override
    {
	    RESET_FAKE(safe_malloc);
        RESET_FAKE(safe_free);
        RESET_FAKE(r_err);
        RESET_FAKE(r_warn);
        RESET_FAKE(json_tostring);
        RESET_FAKE(json_parser_create);
        RESET_FAKE(json_parser_destroy);
        RESET_FAKE(json_parser_eval);
        RESET_FAKE(clock_timestamp);
	}

	void TearDown() override
    {
	}
};

TEST_F(data_tests, new_data_creates_data)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    // Act
    data_t *data = new_data();

    // Assert

    ASSERT_NE(data, nullptr);
    ASSERT_EQ(data->len, 0 );
    delete_data(data);
}

TEST_F(data_tests, delete_data_deletes_data)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    // Act
    data_t *data = new_data();
    delete_data(data);

    // Assert

    ASSERT_NE(data, nullptr);
    ASSERT_EQ(safe_free_fake.call_count, 1 );
}

TEST_F(data_tests, delete_data_null_does_not_delete_data)
{
    // Arrange
    safe_free_fake.custom_fake = safe_free_custom_fake;

    // Act
    data_t *data = nullptr;
    delete_data(data);
    // Assert
    ASSERT_EQ(safe_free_fake.call_count, 0 );
}

TEST_F(data_tests, data_len_returns_len)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    const char* data_string = "string";
    int string_len = strlen(data_string);

    // Act
    data_t *data = new_data();
    data_set_data(data, data_string, string_len);
    int actual_len = data_len(data);

    // Assert
    ASSERT_EQ(actual_len, string_len );
    delete_data(data);
}

TEST_F(data_tests, data_len_returns_0_for_null_data)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    // Act
    data_t *data = nullptr;
    int len = data_len(data);

    // Assert
    ASSERT_EQ(len, 0 );
}

TEST_F(data_tests, data_data_returns_packet_data)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    data_t *data = new_data();
    const char *expected = data->p.data;

    // Act
    const char *actual = data_data(data);
    delete_data(data);

    // Assert
    ASSERT_EQ(actual, expected );
}

TEST_F(data_tests, data_data_returns_NULL_when_data_NULL)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    data_t *data = nullptr;

    const char *expected = nullptr;

    // Act
    const char *actual = data_data(data);


    // Assert
    ASSERT_EQ(actual, expected );
}

TEST_F(data_tests, data_set_data_sets_data)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    data_t *data = new_data();

    std::string expected("string_data");
    int string_len = expected.length();

    // Act
    data_set_data(data, expected.c_str(), string_len);
    const char * actual = data_data(data);

    // Assert
    ASSERT_EQ(std::string(actual), expected );
    delete_data(data);
}

TEST_F(data_tests, data_set_data_too_big_truncates_data_warns)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    data_t *data = new_data();

    const int size = 1600;
    char expected[size];
    memset(expected, 1, size);

    // Act
    data_set_data(data, expected, size);
    int actual_data_len = data_len(data);

    // Assert
    ASSERT_EQ(actual_data_len, DATA_MAXLEN );
    ASSERT_EQ(r_warn_fake.call_count, 1 );
    delete_data(data);
}

TEST_F(data_tests, data_set_data_null_logs_error)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    data_t *data = nullptr;
    std::string expected("string_data");
    int string_len = expected.length();

    // Act
    data_set_data(data, expected.c_str(), string_len);
    int actual_data_len = data_len(data);

    // Assert
    ASSERT_EQ(actual_data_len, 0 );
    ASSERT_EQ(r_err_fake.call_count, 1 );
    delete_data(data);
}

TEST_F(data_tests, data_set_data_null_string_logs_error)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    data_t *data = new_data();;
    std::string expected("string_data");
    int string_len = expected.length();

    // Act
    data_set_data(data, nullptr, string_len);
    int actual_data_len = data_len(data);

    // Assert
    ASSERT_EQ(actual_data_len, 0 );
    ASSERT_EQ(r_err_fake.call_count, 1 );
    delete_data(data);
}

TEST_F(data_tests, data_set_len_sets_len) {
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    data_t *data = new_data();;
    std::string expected("string_data");
    int string_len = expected.length();

    int expected_len = 512;

    // Act
    data_set_data(data, expected.c_str(), string_len);
    int actual_len = data_len(data);
    data_set_len(data, expected_len);
    actual_len = data_len(data);

    // Assert
    ASSERT_EQ(actual_len, expected_len);
    delete_data(data);
}

TEST_F(data_tests, data_set_len_truncates_len)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    data_t *data = new_data();;
    std::string expected("string_data");
    int string_len = expected.length();

    int expected_len = DATA_MAXLEN;

    // Act
    data_set_data(data, expected.c_str(), string_len);
    int actual_len = data_len(data);
    data_set_len(data, (expected_len+100));
    actual_len = data_len(data);

    // Assert
    ASSERT_EQ(actual_len, expected_len);
    delete_data(data);
}

TEST_F(data_tests, data_set_len_negative_sets_len_0)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    data_t *data = new_data();;
    std::string expected("string_data");
    int string_len = expected.length();

    int expected_len = 0;

    // Act
    data_set_data(data, expected.c_str(), string_len);
    int actual_len = data_len(data);
    data_set_len(data, -1);
    actual_len = data_len(data);

    // Assert
    ASSERT_EQ(actual_len, expected_len);
    delete_data(data);
}

TEST_F(data_tests, data_append_zero_appends_zero_increases_len )
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    data_t *data = new_data();
    std::string expected("string_data");
    int string_len = expected.length();

    int expected_len = string_len + 1;

    // Act
    data_set_data(data, expected.c_str(), string_len);
    data_append_zero(data);
    int actual_len = data_len(data);

    // Assert
    ASSERT_EQ(actual_len, expected_len);
    delete_data(data);
}

TEST_F(data_tests, data_append_zero_when_max_inserts_0)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    const int expected_size = DATA_MAXLEN;
    char expected[expected_size];
    memset(expected, 1, expected_size);

    data_t *data = new_data();
    data_set_data(data, expected, expected_size);

    // Act
    data_append_zero(data);
    const char *actual_data = data_data(data);
    const int actual_size = data_len(data);

    // Assert
    std::vector<char> actual_vector(actual_data, actual_data + actual_size);
    std::vector<char> expected_vector(expected, expected + expected_size);
    expected_vector[expected_size-1] = 0;

    EXPECT_THAT(actual_vector, ::testing::ContainerEq(expected_vector));
    delete_data(data);
}

TEST_F(data_tests, data_printf_prints_data_to_buffer)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    const char * data_string = "data_string";
    const int number = 5;
    const char *format_string = "This is a %s this is a number %d";
    std::string expected_string("This is a data_string this is a number 5");
    data_t *data = new_data();

    // Act
    data_printf(data, format_string, data_string, number);
    const char *actual_data = data_data(data);
    std::string actual_string(actual_data);

    // Assert
    ASSERT_EQ(actual_string, expected_string);
    delete_data(data);
}

TEST_F(data_tests, data_printf_truncates_long_buffer_prints_data_to_buffer)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;

    const int data_string_large_size = 1500;
    char data_string[data_string_large_size];
    memset(data_string, 'a', data_string_large_size);
    data_string[data_string_large_size-1] = 0;

    const int expected_data_string_size = DATA_MAXLEN;
    char expected_data_string[expected_data_string_size];
    memset(expected_data_string, 0, expected_data_string_size);
    memset(expected_data_string, 'a', expected_data_string_size-1); // NULL.

    data_t *data = new_data();

    // Act
    data_printf(data, "%s", data_string);
    const char *actual_data = data_data(data);
    std::string actual_string(actual_data);
    std::string expected_string(expected_data_string);

    // Assert
    ASSERT_EQ(actual_string, expected_string);
    delete_data(data);
}

TEST_F(data_tests, data_serialise_serialises_to_buffer)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    json_tostring_fake.return_val = 0;

    data_t *data = new_data();
    json_object_t json_obj = json_object_create();

    // Act
    int actual_result = data_serialise(data, json_obj);

    // Assert
    ASSERT_EQ(actual_result, 0);
    ASSERT_EQ(json_tostring_fake.call_count, 1);
    ASSERT_EQ(json_tostring_fake.arg0_val, json_obj);
    ASSERT_EQ(json_tostring_fake.arg2_val, data->p.data);

    delete_data(data);
    json_unref(json_obj);
}

TEST_F(data_tests, data_serialise_when_serialise_fails_logs_returns_error)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    json_tostring_fake.return_val = -1;

    data_t *data = new_data();
    json_object_t json_obj = json_object_create();

    // Act
    int actual_result = data_serialise(data, json_obj);

    // Assert
    ASSERT_EQ(actual_result, -1);
    ASSERT_EQ(json_tostring_fake.call_count, 1);
    ASSERT_EQ(r_err_fake.call_count, 1);

    delete_data(data);
    json_unref(json_obj);
}

TEST_F(data_tests, data_parse_if_parser_NULL_parser_created_and_destroyed)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    data_t *data = new_data();

    // Act
    data_parse(data, nullptr);

    // Assert
    ASSERT_EQ(json_parser_create_fake.call_count, 1);
    ASSERT_EQ(json_parser_destroy_fake.call_count, 1);
    delete_data(data);
}


TEST_F(data_tests, data_parse_if_parser_not_NULL_parser_not_created_or_destroyed)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    data_t *data = new_data();
    json_parser_t* parser = (json_parser_t*)data;

    // Act
    data_parse(data, parser);

    // Assert
    ASSERT_EQ(json_parser_create_fake.call_count, 0);
    ASSERT_EQ(json_parser_destroy_fake.call_count, 0);
    delete_data(data);
}

TEST_F(data_tests, data_parse_data_parsed)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    json_parser_eval_fake.return_val = nullptr;

    data_t *data = new_data();
    json_parser_t* parser = (json_parser_t*)data;

    // Act
    auto actual = data_parse(data, parser);

    // Assert
    ASSERT_EQ(json_parser_eval_fake.call_count, 1);
    ASSERT_EQ(json_parser_eval_fake.arg0_val, parser);
    ASSERT_EQ(json_parser_eval_fake.arg1_val, data->p.data);
    ASSERT_EQ(actual, nullptr);
    delete_data(data);
}

TEST_F(data_tests, data_packet_returns_packet)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    std::string expected("string8");

    data_t *data = new_data();
    strncpy(data->p.data, expected.c_str(), sizeof(data->p.data));

    // Act
    std::string actual(data_packet(data)->data);

    // Assert
    ASSERT_EQ(actual, expected);
    delete_data(data);
}

TEST_F(data_tests, data_set_timestamp_sets_timestamp)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    uint64_t timestamp = 0x0102030405060708;
    clock_timestamp_fake.return_val = timestamp;

    data_t *data = new_data();

    // Act
    data_set_timestamp(data);
    std::vector<char> timstamp_vector(data->p.timestamp, data->p.timestamp+sizeof(data->p.timestamp));

    // Assert
    ASSERT_THAT(timstamp_vector, ::testing::ElementsAre(0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08));
    delete_data(data);
}

TEST_F(data_tests, data_timestamp_gets_timestamp)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    uint64_t expected = 0x0102030405060708;
    clock_timestamp_fake.return_val = expected;

    data_t *data = new_data();

    // Act
    data_set_timestamp(data);
    auto actual = data_timestamp(data);

    // Assert
    ASSERT_EQ(actual, expected);
    delete_data(data);
}

TEST_F(data_tests, data_clear_timestamp_clears_timestamp)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    uint64_t timestamp = 0x0102030405060708;
    clock_timestamp_fake.return_val = timestamp;

    uint64_t expected = (uint64_t )0;
    data_t *data = new_data();

    // Act
    data_set_timestamp(data);
    auto old_timestamp = data_timestamp(data);
    data_clear_timestamp(data);
    auto actual = data_timestamp(data);

    // Assert
    ASSERT_EQ(old_timestamp, timestamp);
    ASSERT_EQ(actual, expected);
    delete_data(data);
}

TEST_F(data_tests, data_set_seqnum_sets_seqnum)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    uint32_t seqnum = 0x01020304;

    data_t *data = new_data();

    // Act
    data_set_seqnum(data, seqnum);
    std::vector<char> seqnum_vector(data->p.seqnum, data->p.seqnum+sizeof(data->p.seqnum));

    // Assert
    ASSERT_THAT(seqnum_vector, ::testing::ElementsAre(0x01, 0x02, 0x03, 0x04));
    delete_data(data);
}

TEST_F(data_tests, data_get_seqnum_gets_seqnum)
{
    // Arrange
    safe_malloc_fake.custom_fake = safe_malloc_custom_fake;
    safe_free_fake.custom_fake = safe_free_custom_fake;
    uint32_t expected = 0x01020304;

    data_t *data = new_data();

    // Act
    data_set_seqnum(data, expected);
    uint32_t actual = data_seqnum(data);

    // Assert
    ASSERT_THAT(actual, expected);
    delete_data(data);
}
