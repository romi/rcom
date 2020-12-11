
cmake_minimum_required(VERSION 2.6)

project(rcom_examples CXX C)

add_dependencies(rcgen rcom)

## Example apps
#
# binread
message( "BinRead runtime dir = :" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/binread/binread_main.c
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rcgen code ${CMAKE_CURRENT_SOURCE_DIR}/binread/binread_main.c ${CMAKE_CURRENT_SOURCE_DIR}/binread/binread.json
  DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rcgen ${CMAKE_CURRENT_SOURCE_DIR}/binread/binread.json
  )

add_executable(binread_example ${CMAKE_CURRENT_SOURCE_DIR}/binread/binread.c ${CMAKE_CURRENT_SOURCE_DIR}/binread/binread_main.c)
target_link_libraries(binread_example rcom)

# binwrite
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/binwrite/binwrite_main.c
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rcgen code ${CMAKE_CURRENT_SOURCE_DIR}/binwrite/binwrite_main.c ${CMAKE_CURRENT_SOURCE_DIR}/binwrite/binwrite.json
  DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rcgen ${CMAKE_CURRENT_SOURCE_DIR}/binwrite/binwrite.json
  )
add_executable(binwrite_example ${CMAKE_CURRENT_SOURCE_DIR}/binwrite/binwrite.c ${CMAKE_CURRENT_SOURCE_DIR}/binwrite/binwrite_main.c)
target_link_libraries(binwrite_example rcom )

## camera
ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/camera/camera_main.c
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rcgen code ${CMAKE_CURRENT_SOURCE_DIR}/camera/camera_main.c ${CMAKE_CURRENT_SOURCE_DIR}/camera/camera.json
        DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rcgen ${CMAKE_CURRENT_SOURCE_DIR}/camera/camera.json
)

add_executable(camera_example ${CMAKE_CURRENT_SOURCE_DIR}/camera/video4linux.c ${CMAKE_CURRENT_SOURCE_DIR}/camera/camera_impl.c ${CMAKE_CURRENT_SOURCE_DIR}/camera/camera_main.c)
target_link_libraries(camera_example
                        rcom
                        jpeg)

# camera_listener
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/camera_listener/camera_listener_main.c
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rcgen code ${CMAKE_CURRENT_SOURCE_DIR}/camera_listener/camera_listener_main.c ${CMAKE_CURRENT_SOURCE_DIR}/camera_listener/camera_listener.json
  DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rcgen ${CMAKE_CURRENT_SOURCE_DIR}/camera_listener/camera_listener.json
  )

add_executable(camera_listener_example ${CMAKE_CURRENT_SOURCE_DIR}/camera_listener/camera_listener.c ${CMAKE_CURRENT_SOURCE_DIR}/camera_listener/camera_listener_main.c)
target_link_libraries(camera_listener_example rcom)

# motorcontroller
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/motorcontroller/motorcontroller_main.c
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rcgen code ${CMAKE_CURRENT_SOURCE_DIR}/motorcontroller/motorcontroller_main.c ${CMAKE_CURRENT_SOURCE_DIR}/motorcontroller/motorcontroller.json
  DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rcgen ${CMAKE_CURRENT_SOURCE_DIR}/motorcontroller/motorcontroller.json
  )

add_executable(motorcontroller_example ${CMAKE_CURRENT_SOURCE_DIR}/motorcontroller/motorcontroller.c ${CMAKE_CURRENT_SOURCE_DIR}/motorcontroller/motorcontroller_main.c)
target_link_libraries(motorcontroller_example rcom)

# wheel_odometry
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/wheel_odometry/wheel_odometry_main.c
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rcgen code ${CMAKE_CURRENT_SOURCE_DIR}/wheel_odometry/wheel_odometry_main.c ${CMAKE_CURRENT_SOURCE_DIR}/wheel_odometry/wheel_odometry.json
  DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rcgen ${CMAKE_CURRENT_SOURCE_DIR}/wheel_odometry/wheel_odometry.json
  )

add_executable(wheel_odometry_example ${CMAKE_CURRENT_SOURCE_DIR}/wheel_odometry/wheel_odometry.c ${CMAKE_CURRENT_SOURCE_DIR}/wheel_odometry/wheel_odometry_main.c)
target_link_libraries(wheel_odometry_example rcom)

