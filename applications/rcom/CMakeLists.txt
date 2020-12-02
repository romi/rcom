cmake_minimum_required(VERSION 3.10)

include_directories(include)

add_executable(rcom
                src/rcom.c)

target_link_libraries(rcom
                        rcomlib)

INSTALL(TARGETS
            rcom
        DESTINATION
            "bin")

#if(BUILD_TESTS)
#    add_subdirectory(test)
#endif()
