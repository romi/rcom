import registry
import time

registry_tests = registry.RegistryTests(True)

for i in range(1000):

    print()
    print("*** Test %d ***" % i)
    if registry_tests.registry_run(12000+i) is True:
        if registry_tests.registry_connect() is True:
            registry_tests.registry_send_test("{'request':'list'}", True, "OK")
        registry_tests.registry_terminate()
    time.sleep(1)
