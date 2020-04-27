
cmake_minimum_required(VERSION 2.6)
project(rcom)

# The version number.
set(rcom_VERSION_MAJOR 0)
set(rcom_VERSION_MINOR 1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

############################################################

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

############################################################

include(${CMAKE_MODULES_DIRECTORY}/CodeCoverage.cmake)
set( COVERAGE_EXCLUDES
        "/usr/*"
        "*/googletest/*"
        "*/googlemock/*"
        "*/thirdparty*"
        "*test/*")

# Options.
#Build tests by default. We want run tests whether we are developing or building a release. I mean... why wouldn't you? :)
option(BUILD_TESTS "Build all tests." ON)
option(BUILD_COVERAGE "Build coverage." ON)

if(BUILD_TESTS)
    set(CMAKE_MODULES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
    set(CMAKE_THIRDPARTY_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)

    include(CTest)
    enable_testing()

    if (NOT DEFINED googletest_SOURCE_DIR)
        if (CMAKE_VERSION VERSION_LESS 3.2)
            set(UPDATE_DISCONNECTED_IF_AVAILABLE "")
        else()
            set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")
        endif()

        include(${CMAKE_MODULES_DIRECTORY}/DownloadProject.cmake)
        download_project(   PROJ                googletest
                GIT_REPOSITORY      https://github.com/google/googletest.git
                GIT_TAG             master
                PREFIX              ${CMAKE_THIRDPARTY_DIRECTORY}/googletest
                ${UPDATE_DISCONNECTED_IF_AVAILABLE}
                )

        # Prevent GoogleTest from overriding our compiler/linker options
        # when building with Visual Studio
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
    endif()

endif() # BUILD_TESTS

# Can't build for coverage without the test.
if (BUILD_COVERAGE AND BUILD_TESTS)
    if (NOT DEFINED lcov_SOURCE_DIR)
        # lcov doesn't build with CMake so we download, set lcov as an external target, then call make.
        # we then manually add the bin path so that we can call it later.
        download_project(   PROJ              lcov
                URL               https://github.com/linux-test-project/lcov/releases/download/v1.14/lcov-1.14.tar.gz
                PREFIX            ${CMAKE_THIRDPARTY_DIRECTORY}/lcov
                TIMEOUT           180
                ${UPDATE_DISCONNECTED_IF_AVAILABLE}
                )

        set(LCOV_PATH "${lcov_SOURCE_DIR}/bin/lcov")
        set(GENHTML_PATH "${lcov_SOURCE_DIR}/bin/genhtml")
    endif()

    append_coverage_compiler_flags()
endif()

######################################

if (NOT DEFINED libr_SOURCE_DIR)
    download_project(
            PROJ              libr
            GIT_REPOSITORY    https://github.com/romi/libr.git
            GIT_TAG           ci_dev
            TIMEOUT           180
            SOURCE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/libr
            BINARY_DIR        ${CMAKE_CURRENT_BINARY_DIR}/libr
            ${UPDATE_DISCONNECTED_IF_AVAILABLE})

    add_subdirectory(${libr_SOURCE_DIR} ${libr_BINARY_DIR})
endif()

###########################################################3

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


if(BUILD_TESTS)
    add_subdirectory(test)
endif()

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
