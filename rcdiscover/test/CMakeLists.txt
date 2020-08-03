cmake_minimum_required(VERSION 3.10)

include_directories(mocks
                        support)

set(SRCS
        support/TestUtil.hpp
        src/tests_main.cpp
        src/SerialPortIdentification_tests.cpp
        src/SerialPortDiscover_tests.cpp
        ../include/ISerialPortIdentification.h
        ../include/SerialPortIdentification.h
        ../src/SerialPortIdentification.cpp
        ../include/ISerialPortDiscover.h
        ../include/SerialPortDiscover.h
        ../src/SerialPortDiscover.cpp)

add_executable(rcdiscover_unit_tests ${SRCS})

target_link_libraries(  rcdiscover_unit_tests
                        gtest
                        gmock
                        rcom
                        stdc++fs)

add_test(
        NAME rcdiscover_unit_tests
        COMMAND rcdiscover_unit_tests
        )
