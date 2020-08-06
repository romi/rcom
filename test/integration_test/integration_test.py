import random
import registry
import sys
import traceback

test_return_value = 1
OK = "OK"
Unknown_Request = "Unknown request"
test_array = []

# Construct registry tests with logging value
registry_tests = registry.RegistryTests(False)


# Array Format:
#       Function,       send_data,      success_return_value,       message_string
def add_tests():
    global test_array
    test_array = [
        (registry_tests.registry_send_test, "{'request':'xxxx'}", False, Unknown_Request),
        (registry_tests.registry_send_test, "{'request':'list'}", True,  OK),
        (registry_tests.registry_send_test, "{'request':'xxxx'}", False, Unknown_Request),
        (registry_tests.registry_send_test, "{'request':'list'}", True,  OK),
        # Following test will fail.
        # (registry_tests.registry_send_test, "{'request':'xxxx'}", False, "FAILED"),
    ]


def run_tests():
    global test_array
    for test in test_array:
        function_name, send_data, success_value, message_value = test
        result = function_name(send_data, success_value, message_value)
        if result != 0:
            return 1
    return 0


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
