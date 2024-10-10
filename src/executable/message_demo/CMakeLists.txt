add_executable(message_demo main.cpp)


target_include_directories(message_demo
    PUBLIC
        ${_network_dir}/src
        ${_system_dir}/src
        ${_work_contract_dir}/src
)


target_link_directories(message_demo PRIVATE ${CMAKE_BINARY_DIR}/lib)

target_link_libraries(message_demo 
PRIVATE
  message
  network
  system
  work_contract
)