add_executable(message_demo main.cpp)

target_link_libraries(message_demo 
PRIVATE
  message
  network
  system
)