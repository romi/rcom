import random
import registry
import sys

test_return_value = 1

registry_tests = registry.RegistryTests()

connection_port = random.randrange(2000, 65535)
if registry_tests.registry_run(connection_port) is True:
    if registry_tests.registry_connect() is True:
        test_return_value = registry_tests.registry_send_test()

registry_tests.registry_terminate()
sys.exit(test_return_value)
