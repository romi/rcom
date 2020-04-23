cmake_minimum_required(VERSION 3.10)

set(SRCS
    src/tests_main.cpp
    src/addr_tests.cpp)

add_executable(rcom_unit_tests ${SRCS})

target_link_libraries(  rcom_unit_tests
                        gtest
                        gmock
                        rcom)

add_test(
        NAME rcom_unit_tests
        COMMAND rcom_unit_tests
        )

if (BUILD_COVERAGE)

        SETUP_TARGET_FOR_COVERAGE_LCOV(
                                    NAME rcom_unit_tests_coverage
                                    EXECUTABLE ctest -V ${n_cores}
                                    DEPENDENCIES rcom_unit_tests )
endif()