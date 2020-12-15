cmake_minimum_required(VERSION 3.10)

include_directories(include)

add_executable(rclaunch
        src/rclaunch.cpp
                src/run.c)

target_link_libraries(rclaunch
                        rcom)

INSTALL(TARGETS
            rclaunch
        DESTINATION
            "bin")

#if(BUILD_TESTS)
#    add_subdirectory(test)
#endif()
