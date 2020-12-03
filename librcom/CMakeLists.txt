
cmake_minimum_required(VERSION 2.6)
project(librcom)


set(SOURCES
        src/addr.c
        src/net.c
        src/util.c
        src/response.c
        src/http.c
        src/http_parser.c
        src/sha1.c
        src/http.c
        src/app.c
        src/circular.c
        src/data.c
        src/datalink.c
        src/datahub.c
        src/messagehub.c
        src/messagelink.c
        src/rcregistry.c
        src/registry.c
        src/proxy.c
        src/dump.c
        src/export.c
        src/request.c
        src/service.c
        src/client.c
        src/streamer.c
        src/streamerlink.c
        src/multipart_parser.c
        src/dump.c
  )

add_library( librcom SHARED ${SOURCES})

# Note we keep the private folder out of the public export.
target_include_directories(
        librcom
        PUBLIC
#              ${CMAKE_CURRENT_SOURCE_DIR}/include
            include/rcom
        PRIVATE
            include/private
        )

target_link_libraries( librcom
                      m
                      r )

if(BUILD_TESTS)
    add_subdirectory(test)
endif()

