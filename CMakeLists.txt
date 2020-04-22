
cmake_minimum_required(VERSION 2.6)
project(rcom)

#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# The version number.
set(rcom_VERSION_MAJOR 0)
set(rcom_VERSION_MINOR 1)

#set(Python_ADDITIONAL_VERSIONS 3.6)
#find_package(PythonLibs 3 REQUIRED)

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
  src/registry.c 
  src/proxy.c 
  src/rcregistry.c 
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

add_library(rcom SHARED ${SOURCES})

target_include_directories(rcom
        PUBLIC
              "${CMAKE_CURRENT_SOURCE_DIR}/include"
              "${CMAKE_CURRENT_SOURCE_DIR}/include/rcom"
        )

target_link_libraries(rcom
                      m
                      r )

# Main rcom apps
add_executable(rcregistry src/rcregistry_main.c)
target_link_libraries(rcregistry rcom)

add_executable(rcgen src/rcgen.c)
target_link_libraries(rcgen rcom)

add_executable(rcom-bin src/rcquery.c)
target_link_libraries(rcom-bin rcom)
set_target_properties(rcom-bin PROPERTIES OUTPUT_NAME rcom)

add_executable(rclaunch src/rclaunch.c src/run.c)
target_link_libraries(rclaunch rcom)

#add_executable(rcreplay src/rcreplay.c)
#target_link_libraries(rcreplay rcom)

#add_executable(rcconvert src/rcconvert.c)
#target_link_libraries(rcconvert rcom)

#add_executable(rcdev src/rcdev.c)
#target_link_libraries(rcdev rcom usb-1.0)


## Installed files
INSTALL(TARGETS rcom DESTINATION "lib")
INSTALL(TARGETS rcom-bin DESTINATION "bin")
INSTALL(TARGETS rcregistry DESTINATION "bin")
INSTALL(TARGETS rcgen DESTINATION "bin")
INSTALL(TARGETS rclaunch DESTINATION "bin")
#INSTALL(TARGETS rcreplay DESTINATION "bin")
#INSTALL(TARGETS rcconvert DESTINATION "bin")
#
INSTALL (
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h")
