#pragma once

#include <library/message.h>

#include <cstdint>


enum class my_message_indicator : std::uint8_t
{
    login_request = 1,
    login_response = 2
};


using my_protocol = bcpp::message::protocol
        <
            bcpp::message::protocol_traits<"my_protocol_name", {1, 0, 'a'}, my_message_indicator>, 
            my_message_indicator::login_request,
            my_message_indicator::login_response
        >;



namespace bcpp::message
{

    template <>
    struct message_header<my_protocol>
    {
        message_header(my_message_indicator messageIndicator, std::uint16_t size):messageIndicator_(messageIndicator), size_(size){}
        
        auto get_message_indicator() const{return messageIndicator_;}
        auto size() const{return size_;}

        my_message_indicator messageIndicator_;
        std::uint16_t        size_;
    };


    template <>
    struct message<my_protocol, my_message_indicator::login_request> :
        message_header<my_protocol>
    {
        message():message_header(my_message_indicator::login_request, sizeof(*this)){};

        std::array<char, 32> account_;
        std::array<char, 32> password_;
    };


    template <>
    struct message<my_protocol, my_message_indicator::login_response> :
        message_header<my_protocol>
    {
        message():message_header(my_message_indicator::login_response, sizeof(*this)){};

        enum class response_code : std::uint8_t
        {
            undefined = 0,
            success = 1,
            invalid_account = 2,
            invalid_password = 3
        };
        response_code responseCode_;
    };
}


using login_request_message = bcpp::message::message<my_protocol, my_message_indicator::login_request>;
using login_response_message = bcpp::message::message<my_protocol, my_message_indicator::login_response>;
