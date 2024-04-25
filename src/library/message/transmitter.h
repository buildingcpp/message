#pragma once

#include "./message.h"
#include "./pipe/pipe.h"


namespace bcpp::message 
{

    template <packet_concept T>
    class transmitter final
    {
    public:

        using packet = T;
        using protocol = typename packet::protocol;

        static auto constexpr default_packet_capacity = ((1 << 10) * 2);

        struct configuration
        {
            std::size_t packetCapacity_ = default_packet_capacity;
        };

        using packet_allocate_handler = std::function<packet(transmitter const &, std::size_t)>;
        using packet_handler = std::function<void(transmitter const &, packet)>;

        struct event_handlers
        {
            packet_allocate_handler     packetAllocateHandler_;
            packet_handler              packetHandler_;
        };

        transmitter
        (
            configuration const &,
            event_handlers const &
        );

        bool send
        (
            message_concept auto const & message
        ) requires (std::is_same_v<protocol, typename std::decay_t<decltype(message)>::protocol>);

        void flush();

    private:

        packet_allocate_handler     packetAllocateHandler_;

        packet_handler              packetHandler_;

        std::size_t                 packetCapacity_;

        packet                      packet_;

    }; // class transmitter

} // namespace bcpp::message


//=============================================================================
template <bcpp::message::packet_concept T>
bcpp::message::transmitter<T>::transmitter
(
    configuration const & config,
    event_handlers const & eventHandlers
):
    packetAllocateHandler_(eventHandlers.packetAllocateHandler_ ? eventHandlers.packetAllocateHandler_ : [](auto const &, std::size_t capacity){return packet(capacity);}),
    packetHandler_(eventHandlers.packetHandler_ ? eventHandlers.packetHandler_ : [](auto const &, auto){}),
    packetCapacity_((config.packetCapacity_ == 0) ? config.packetCapacity_ : default_packet_capacity)
{
}


//=============================================================================
template <bcpp::message::packet_concept T>
bool bcpp::message::transmitter<T>::send
(
    message_concept auto const & message
) requires (std::is_same_v<protocol, typename std::decay_t<decltype(message)>::protocol>)
{
    if (auto success = packet_.insert(message); success)
        return true;
    flush();
    return packet_.insert(message);
}


//=============================================================================
template <bcpp::message::packet_concept T>
void bcpp::message::transmitter<T>::flush
(
)
{
    if (!packet_.empty())
        packetHandler_(*this, std::move(packet_));
    packet_ = packetAllocateHandler_(*this, packetCapacity_);
}
