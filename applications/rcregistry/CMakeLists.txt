cmake_minimum_required(VERSION 3.10)

include_directories(include)

add_executable(rcregistry
                src/rcregistry_main.c)

target_link_libraries(rcregistry
                        rcom)

INSTALL(TARGETS
            rcregistry
        DESTINATION "bin")

#if(BUILD_TESTS)
#    add_subdirectory(test)
#endif()