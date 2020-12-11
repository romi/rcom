cmake_minimum_required(VERSION 3.10)

include_directories(include)

add_executable(rcutil
        src/rcutil.c)

target_link_libraries(rcutil
                        rcom)

INSTALL(TARGETS
            rcutil
        DESTINATION
            "bin")

#if(BUILD_TESTS)
#    add_subdirectory(test)
#endif()
