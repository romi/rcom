cmake_minimum_required(VERSION 3.10)

include_directories(include)

add_executable(rcgen
        src/rcgen.cpp)

target_link_libraries(rcgen
                        rcom)

INSTALL(TARGETS
            rcgen
        DESTINATION
            "bin")

#if(BUILD_TESTS)
#    add_subdirectory(test)
#endif()
