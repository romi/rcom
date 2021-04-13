
cmake_minimum_required(VERSION 2.6)
project(librcompp-examples)

add_executable(hello-world-server rpc-hello-world/hello-world-server.cpp)
target_link_libraries(hello-world-server rcompp-lite rpp r m)
INSTALL(TARGETS hello-world-server DESTINATION "bin")

add_executable(hello-world-client rpc-hello-world/hello-world-client.cpp)
target_link_libraries(hello-world-client rcompp-lite rpp r m)
INSTALL(TARGETS hello-world-client DESTINATION "bin")


