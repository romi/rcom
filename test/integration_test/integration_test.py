import random
import registry
import sys
import traceback

test_return_value = 1
# Error strings
OK = "OK"
Unknown_Request = "Unknown request"
Invalid_Id = "Invalid ID"
Invalid_Topic = "Invalid topic"
Invalid_Type = "Invalid type"
Invalid_Address = "Invalid address"

test_array = []

# Construct registry tests with logging value
registry_tests = registry.RegistryTests(False)


# Array Format:
#       Function,       send_data,      success_return_value,       message_string
def add_tests():
    global test_array
    test_array = [
        (registry_tests.registry_send_test, "{'request':'xxxx'}", False, Unknown_Request),
        (registry_tests.registry_send_test, "{'request':'list'}", True, OK),
        (registry_tests.registry_send_test, "{'request':'register'}", False, Invalid_Id),
        (registry_tests.registry_send_test, "{'request':'register','entry':{'id':'mock'}}", False, Invalid_Id),
        (registry_tests.registry_send_test, "{'request':'register','entry':{'id':'1b4e28ba-2fa1-11d2-883f-0016d3cca427', 'name':'mocker'}}", False, Invalid_Topic),
        (registry_tests.registry_send_test, "{'request':'register','entry':{'id':'1b4e28ba-2fa1-11d2-883f-0016d3cca427', 'name':'mocker', 'topic':'registry'}}", False, Invalid_Type),
        (registry_tests.registry_send_test, "{'request':'register','entry':{'id':'1b4e28ba-2fa1-11d2-883f-0016d3cca427', 'name':'mocker', 'topic':'registry', 'type':'messagelink'}}", False, Invalid_Address),
        # Following test will fail.
        # (registry_tests.registry_send_test, "{'request':'xxxx'}", False, "FAILED"),
    ]


def run_tests():
    global test_array
    error_result = 0
    for test in test_array:
        function_name, send_data, success_value, message_value = test
        test_result = function_name(send_data, success_value, message_value)
        if test_result != 0:
            error_result = 1
    return error_result


def main():
    global test_return_value
    add_tests()
    try:
        connection_port = random.randrange(2000, 65535)
        if registry_tests.registry_run(connection_port) is True:
            if registry_tests.registry_connect() is True:
                test_return_value = run_tests()
    except Exception:
        traceback.print_exc()
    finally:
        registry_tests.registry_terminate()
        sys.exit(test_return_value)


if __name__ == "__main__":
    main()
