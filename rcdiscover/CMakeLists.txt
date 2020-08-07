cmake_minimum_required(VERSION 3.10)

include_directories(include
                    src)

add_executable(rcdiscover
                src/main.cpp
                include/SerialPortIdentification.h
                src/SerialPortIdentification.cpp
                include/SerialPortDiscover.h
                src/SerialPortDiscover.cpp
                include/SerialPortConfigurationGenerator.h
                src/SerialPortConfigurationGenerator.cpp)

target_link_libraries( rcdiscover
                        stdc++fs
                        r)

if(BUILD_TESTS)
    add_subdirectory(test)
endif()