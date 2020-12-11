cmake_minimum_required(VERSION 3.10)

include_directories(include)

add_executable(rcom
        src/rcutil.c)

target_link_libraries(rcom
                        librcom)

INSTALL(TARGETS
            rcom
        DESTINATION
            "bin")

#if(BUILD_TESTS)
#    add_subdirectory(test)
#endif()
