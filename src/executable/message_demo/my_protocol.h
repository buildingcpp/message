#pragma once

#include <library/message.h>
#include <include/endian.h>

#include <cstdint>
#include <array>


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

    //=========================================================================
    // specialize header which is common for all messages within the protocol
    #pragma pack(push, 1)
    template <>
    struct message_header<my_protocol>
    {
        using protocol = my_protocol;
        message_header(my_message_indicator messageIndicator, std::uint16_t size):messageIndicator_(messageIndicator), size_(size){}
        auto get_message_indicator() const{return messageIndicator_;}
        auto size() const{return size_;}
        my_message_indicator messageIndicator_;
        std::uint16_t        size_;
    };
    #pragma pack(pop)


    //=========================================================================
    // specialize a login request message
    // for demonstration purposes add payload methods
    #pragma pack(push, 1)
    template <>
    struct message<my_protocol, my_message_indicator::login_request> :
        message_header<my_protocol>
    {
        static auto constexpr type = my_message_indicator::login_request;
        message():message_header(type, sizeof(*this)){};
        message(std::string_view const account, std::string_view const password):message_header(type, sizeof(*this))
        {
            std::memcpy(account_.data(), account.data(), account.size());
            std::memcpy(password_.data(), password.data(), password.size());
        }
        std::array<char, 32> account_{};
        std::array<char, 32> password_{};
    };
    #pragma pack(pop)


    //=========================================================================
    // specialize a login response message
    #pragma pack(push)
    template <>
    struct message<my_protocol, my_message_indicator::login_response> :
        message_header<my_protocol>
    {
        static auto constexpr type = my_message_indicator::login_response;
        
        enum class response_code : std::uint8_t
        {
            undefined = 0,
            success = 1,
            invalid_account = 2,
            invalid_password = 3
        };

        message():message_header(type, sizeof(*this)){}

        message(response_code responseCode):message_header(type, sizeof(*this)), responseCode_(responseCode){}

        response_code responseCode_;
    };
    #pragma pack(pop)

} // namespace bcpp::message


using login_request_message = bcpp::message::message<my_protocol, my_message_indicator::login_request>;
using login_response_message = bcpp::message::message<my_protocol, my_message_indicator::login_response>;
