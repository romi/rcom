
cmake_minimum_required(VERSION 2.6)
project(rcom)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_C_FLAGS "-O0 -ggdb")
#set(CMAKE_C_FLAGS_DEBUG "-O0 -ggdb -DMEM_DIAGNOSTICS=1")
set(CMAKE_C_FLAGS_DEBUG "-O0 -ggdb")
set(CMAKE_C_FLAGS_RELEASE "-O0 -ggdb")

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

include_directories(include)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(rcom SHARED ${SOURCES})
target_link_libraries(rcom m)
target_link_libraries(rcom r)

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

## Example apps

# binread
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_SOURCE_DIR}/examples/binread/binread_main.c
  COMMAND rcgen code ${CMAKE_SOURCE_DIR}/examples/binread/binread_main.c ${CMAKE_SOURCE_DIR}/examples/binread/binread.json
  DEPENDS rcgen ${CMAKE_SOURCE_DIR}/examples/binread/binread.json
  )

add_executable(binread examples/binread/binread.c examples/binread/binread_main.c)
target_link_libraries(binread rcom)


# binwrite
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_SOURCE_DIR}/examples/binwrite/binwrite_main.c
  COMMAND rcgen code ${CMAKE_SOURCE_DIR}/examples/binwrite/binwrite_main.c ${CMAKE_SOURCE_DIR}/examples/binwrite/binwrite.json
  DEPENDS rcgen ${CMAKE_SOURCE_DIR}/examples/binwrite/binwrite.json
  )
add_executable(binwrite examples/binwrite/binwrite.c examples/binwrite/binwrite_main.c)
target_link_libraries(binwrite rcom)


# camera
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_SOURCE_DIR}/examples/camera/camera_main.c
  COMMAND rcgen code ${CMAKE_SOURCE_DIR}/examples/camera/camera_main.c ${CMAKE_SOURCE_DIR}/examples/camera/camera.json
  DEPENDS rcgen ${CMAKE_SOURCE_DIR}/examples/camera/camera.json
  )

add_executable(video4linux examples/camera/video4linux.c examples/camera/camera_impl.c examples/camera/camera_main.c)
target_link_libraries(video4linux rcom)
target_link_libraries(video4linux jpeg)


# camera_listener
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_SOURCE_DIR}/examples/camera_listener/camera_listener_main.c
  COMMAND rcgen code ${CMAKE_SOURCE_DIR}/examples/camera_listener/camera_listener_main.c ${CMAKE_SOURCE_DIR}/examples/camera_listener/camera_listener.json
  DEPENDS rcgen ${CMAKE_SOURCE_DIR}/examples/camera_listener/camera_listener.json
  )

add_executable(camera_listener examples/camera_listener/camera_listener.c examples/camera_listener/camera_listener_main.c)
target_link_libraries(camera_listener rcom)


# motorcontroller
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_SOURCE_DIR}/examples/motorcontroller/motorcontroller_main.c
  COMMAND rcgen code ${CMAKE_SOURCE_DIR}/examples/motorcontroller/motorcontroller_main.c ${CMAKE_SOURCE_DIR}/examples/motorcontroller/motorcontroller.json
  DEPENDS rcgen ${CMAKE_SOURCE_DIR}/examples/motorcontroller/motorcontroller.json
  )

add_executable(motorcontroller examples/motorcontroller/motorcontroller.c examples/motorcontroller/motorcontroller_main.c)
target_link_libraries(motorcontroller rcom)


# wheel_odometry
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_SOURCE_DIR}/examples/wheel_odometry/wheel_odometry_main.c
  COMMAND rcgen code ${CMAKE_SOURCE_DIR}/examples/wheel_odometry/wheel_odometry_main.c ${CMAKE_SOURCE_DIR}/examples/wheel_odometry/wheel_odometry.json
  DEPENDS rcgen ${CMAKE_SOURCE_DIR}/examples/wheel_odometry/wheel_odometry.json
  )

add_executable(wheel_odometry examples/wheel_odometry/wheel_odometry.c examples/wheel_odometry/wheel_odometry_main.c)
target_link_libraries(wheel_odometry rcom)


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
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h")

