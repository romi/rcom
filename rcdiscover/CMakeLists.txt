cmake_minimum_required(VERSION 3.10)

include_directories(include
                    support)

add_executable(rcdiscover
                main.cpp
                include/SerialPortIdentification.h
                src/SerialPortIdentification.cpp
                include/SerialPortDiscover.h
                src/SerialPortDiscover.cpp)

target_link_libraries( rcdiscover
                        stdc++fs
                        r)

if(BUILD_TESTS)
    add_subdirectory(test)
endif()