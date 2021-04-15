
cmake_minimum_required(VERSION 2.6)
project(librcompp-examples)

add_executable(hello-world-server rpc-hello-world/hello-world-server.cpp)
target_link_libraries(hello-world-server rcompp-lite rpp r m)
INSTALL(TARGETS hello-world-server DESTINATION "bin")

add_executable(hello-world-client rpc-hello-world/hello-world-client.cpp)
target_link_libraries(hello-world-client rcompp-lite rpp r m)
INSTALL(TARGETS hello-world-client DESTINATION "bin")




add_executable(sensor-publisher pubsub-sensor/sensor-publisher.cpp)
target_link_libraries(sensor-publisher rcompp-lite rpp r m)
INSTALL(TARGETS sensor-publisher DESTINATION "bin")

add_executable(sensor-subscriber pubsub-sensor/sensor-subscriber.cpp)
target_link_libraries(sensor-subscriber rcompp-lite rpp r m)
INSTALL(TARGETS sensor-subscriber DESTINATION "bin")




add_executable(chat-server messagebus/chat-server.cpp)
target_link_libraries(chat-server rcompp-lite rpp r m)
INSTALL(TARGETS chat-server DESTINATION "bin")

add_executable(chat-client messagebus/chat-client.cpp)
target_link_libraries(chat-client rcompp-lite rpp r m)
INSTALL(TARGETS chat-client DESTINATION "bin")


