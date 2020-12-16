cmake_minimum_required(VERSION 3.10)


set(SRCS
        src/tests_main.cpp)

add_executable(rcompp_unit_tests ${SRCS})

target_link_libraries(  rcompp_unit_tests
                        gtest
                        gmock
                        rmocks
                        rcompp)

add_test(
        NAME rcompp_unit_tests
        COMMAND rcompp_unit_tests
        )

if(BUILD_COVERAGE)
        SETUP_TARGET_FOR_COVERAGE_LCOV(
                            NAME rcompp_unit_tests_coverage
                            EXECUTABLE ctest -V ${n_cores} -R "rcompp"
                            DEPENDENCIES rcompp_unit_tests )
endif()
