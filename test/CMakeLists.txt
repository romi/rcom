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
add_test(NAME rcom_integration_test
        COMMAND python3 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/integration_test.py )

#COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/integration_test.py WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} )

#find_program (BASH_PROGRAM bash)
#if (BASH_PROGRAM)
#        add_test (
#                NAME rcom_integration_test
#                COMMAND ${BASH_PROGRAM} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/integration_test.py)
#endif (BASH_PROGRAM)

if(BUILD_COVERAGE)
        SETUP_TARGET_FOR_COVERAGE_LCOV(
                            NAME rcom_unit_tests_coverage
                            EXECUTABLE ctest -V ${n_cores}
                            DEPENDENCIES rcom_unit_tests )
endif()