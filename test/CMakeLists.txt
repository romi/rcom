cmake_minimum_required(VERSION 3.10)


set(SRCS
        src/tests_main.cpp
        src/addr_tests.cpp
        src/circular_tests.cpp)

add_executable(rcom_unit_tests ${SRCS})

target_link_libraries(  rcom_unit_tests
                        gtest
                        gmock
                        rmocks
                        rcom)

add_test(
        NAME rcom_unit_tests
        COMMAND rcom_unit_tests
        )

add_custom_command(TARGET rcom_unit_tests POST_BUILD
        COMMAND
        ${CMAKE_COMMAND} -E copy
                ./integration_test/*.py
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        WORKING_DIRECTORY
                ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS
                rcom_unit_tests
        COMMENT
                "Copying test script to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        )

include(FindPythonInterp)
add_test(NAME rcom_integration_tests
        COMMAND python3 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/integration_test.py )

if(BUILD_COVERAGE)
        SETUP_TARGET_FOR_COVERAGE_LCOV(
                            NAME rcom_unit_tests_coverage
                            EXECUTABLE ctest -V ${n_cores}
#                EXECUTABLE ctest -V ${n_cores} -R rcom_unit_tests
                            DEPENDENCIES rcom_unit_tests )
endif()