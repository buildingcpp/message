#include "./my_protocol.h"

#include <array>
#include <string>
#include <iostream>
#include <atomic>
#include <memory>

// a flag used to indicate that the message has been received
std::atomic<bool> done{false};

 // alias for whatever type of container we choose to represent packet
using packet_type = bcpp::message::packet<my_protocol, std::vector<char>>; 

// alias for a pipe containing packets which contain messages from the protocol 'my_protocol' 
// with std::queue used to store incoming packets
using pipe_type = bcpp::message::pipe<my_protocol, std::queue<packet_type>>; 

using message_sender = bcpp::message::transmitter<packet_type>;


//=============================================================================
// a toy class that will receive messages via the input pipe provided to its constructor
class message_recipient : 
    bcpp::message::receiver<message_recipient, my_protocol>
{
public:

    message_recipient
    (
        std::shared_ptr<pipe_type> pipe
    ):
        pipe_(pipe)
    {
    }

    void check_for_messages
    (
        // check for data in the pipe and process the 
        // messages it contains, if any.  Route those messages
        // to the message::receiver 'this'
    )
    {
        if (!pipe_->empty())
            pipe_->process_next_message(*this);
    }

private:

    // friend so that base class can access the messages callbacks in the private section.
    friend class receiver<message_recipient, my_protocol>;

    auto & operator()
    (
        // a message callback used when receiving login_request_messages
        receiver const &,
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
        receiver const &,
        login_response_message const & loginResponseMessage
    )
    {
        std::cout << "Got login response message\n";
        done = true;
        return *this;
    }

    // the incoming pipe containg data (messages) from the protocol 'my_protocol'
    std::shared_ptr<pipe_type> pipe_;
};



//=============================================================================
int main
(
    int,
    char **
)
{
    // create a pipe that we will push messages into 
    // and which, our messages recipient class shall pop messages out of
    auto pipe = std::make_shared<pipe_type>(pipe_type::configuration{}, pipe_type::event_handlers{});
    message_recipient messageRecipient(pipe);
    message_sender messageSender({}, {.packetHandler_ = [&](auto const &, auto packet){*pipe << std::move(packet);}});

    // create a login request message in a 'packet'
    messageSender.send(login_request_message("my_account", "my_password"));
    messageSender.send(login_response_message(login_response_message::response_code::success));

    messageSender.flush();

    // loop until the message recipient class has acked the message that we just pushed into the pipe.
    while (!done)
        messageRecipient.check_for_messages();
  
    return 0;
}
