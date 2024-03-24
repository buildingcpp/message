add_library(message
    ./message.cpp
)


target_link_libraries(message 
PUBLIC
    include
    system
    network
    )

target_include_directories(message
    PUBLIC
        ${_include_dir}/src
        ${_system_dir}/src
        ${_network_dir}/src
        ${_message_dir}/src
)
