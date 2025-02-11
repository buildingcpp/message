#include "./my_protocol.h"

#include <library/network.h>

#include <array>
#include <string>
#include <iostream>
#include <atomic>
#include <memory>

// a flag used to indicate that the message has been received
std::atomic<bool> done{false};

 // alias for whatever type of container we choose to represent packet
using packet_type = std::vector<char>;//bcpp::network::packet; //

using packet_queue_type = std::queue<packet_type>; 

// a message transmitter sending the specified type of packets
using message_sender = bcpp::message::transmitter<my_protocol, packet_type>;


//=============================================================================
// a toy class that will receive messages
class message_recipient : 
    public bcpp::message::receiver<message_recipient, my_protocol, packet_queue_type>
{
public:

    message_recipient():receiver({}, {}){}
    
private:

    friend class receiver; // friend so that base class can access the messages callbacks in the private section.

    auto & operator()
    (
        // a message callback used when receiving login_request_messages
        login_request_message const & loginRequestMessage
    )
    {
        std::cout << "Got login request.  account = " << std::string_view(loginRequestMessage.account_.data(), loginRequestMessage.account_.size()) << 
                ", password = " << std::string_view(loginRequestMessage.password_.data(), loginRequestMessage.password_.size()) << '\n';
        return *this;
    }

    auto & operator()
    (
        // a message callback used when receiving login_response_messages
        login_response_message const & loginResponseMessage
    )
    {
        std::cout << "Got login response message\n";
        done = true;
        return *this;
    }
};


//=============================================================================
int main
(
    int,
    char **
)
{
    message_recipient messageRecipient;
    message_sender messageSender({}, {
            .packetAllocateHandler_ = [](auto const & sender, auto size)
                    {
                        packet_type packet;
                        packet.reserve(size);
                        return packet;
                    }, 
            .packetHandler_ = [&](auto const &, auto packet)
                    {
                        messageRecipient << std::move(packet);
                    }});

    // create a login request message in a 'packet'
    messageSender.send(login_request_message("my_account", "my_password"));
    messageSender.emplace<login_response_message>(login_response_message::response_code::success);

    messageSender.flush();

    // loop until the message recipient class has acked the message that we just sent
    while (!done)
        messageRecipient.process_next_message();
  
    return 0;
}
