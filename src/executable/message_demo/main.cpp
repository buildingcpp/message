#include "./my_protocol.h"

#include <iostream>


//=============================================================================
class my_class : 
    public bcpp::message::receiver<my_class, my_protocol>
{
public:

private:

    friend class receiver; // allow base receiver class to receiver to private 

    my_class & operator()
    (
        receiver const &,
        login_request_message const & loginRequestMessage
    )
    {
        std::cout << "Got login request.  account = " << std::string_view(loginRequestMessage.account_.data(), loginRequestMessage.account_.size()) << 
                ", password = " << std::string_view(loginRequestMessage.password_.data(), loginRequestMessage.password_.size()) << '\n';
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
    my_class myClass;

    // create a login request message
    login_request_message loginRequestMessage;
    std::string account = "my_account";
    std::string password = "my_password";
    std::fill_n(loginRequestMessage.account_.data(), loginRequestMessage.account_.size(), '\0');
    std::copy_n(account.data(), account.size(), loginRequestMessage.account_.data());
    std::fill_n(loginRequestMessage.password_.data(), loginRequestMessage.password_.size(), '\0');
    std::copy_n(password.data(), password.size(), loginRequestMessage.password_.data());

    auto send = [&]<typename T>
    (
        // "send" a message as an arbitrary binary blob.  This is a simple demo
        // so there is no actual stream to send data on.  Instead we simply 
        // hand the blob to the instance that will receive it as if it were via
        // some input stream.
        T const & message
    )
    {
        myClass.process(std::span(reinterpret_cast<std::uint8_t const *>(&message), sizeof(message)));
    };

    send(loginRequestMessage);
    
    return 0;
}
