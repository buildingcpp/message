add_library(message
    ./message.cpp
)


target_link_libraries(message 
PUBLIC
    )

target_include_directories(message
    PUBLIC
        ${_include_dir}/src
        ${_message_dir}/src
)
