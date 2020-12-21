
cmake_minimum_required(VERSION 2.6)
project(librcompp)


set(SOURCES
        include/IRPCClient.h
        include/RPCClient.h
        include/IRPCHandler.h
        include/RPCError.h
        include/RPCServer.h
        src/RPCServer.cpp
        src/RPCClient.cpp
        include/RegistryEntry.h
        src/RegistryEntry.cpp
        include/UuId.h
        src/UuId.cpp
        include/ILink.h
        src/MessageLink.cpp)

add_library( rcompp SHARED ${SOURCES})

# Note we keep the private folder out of the public export.
target_include_directories(
        rcompp
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

target_link_libraries( rcompp
                        rcom
                        rpp)

if(BUILD_TESTS)
    add_subdirectory(test)
endif()

