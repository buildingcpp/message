#pragma once


namespace bcpp::message 
{

    template <protocol_concept P, packet_concept T>
    class transmitter final
    {
    public:

        using packet_type = T;
        using protocol = P;

        static auto constexpr default_packet_capacity = ((1 << 10) * 2);

        struct configuration
        {
            std::size_t packetCapacity_ = default_packet_capacity;
        };

        using packet_allocate_handler = std::function<packet_type(transmitter const &, std::size_t)>;
        using packet_handler = std::function<void(transmitter const &, packet_type)>;

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

        packet_type                 packet_;

    }; // class transmitter

} // namespace bcpp::message


//=============================================================================
template <bcpp::message::protocol_concept P, bcpp::message::packet_concept T>
bcpp::message::transmitter<P, T>::transmitter
(
    configuration const & config,
    event_handlers const & eventHandlers
):
    packetAllocateHandler_(eventHandlers.packetAllocateHandler_ ? eventHandlers.packetAllocateHandler_ : [](auto const &, std::size_t capacity){return packet_type(capacity);}),
    packetHandler_(eventHandlers.packetHandler_ ? eventHandlers.packetHandler_ : [](auto const &, auto){}),
    packetCapacity_((config.packetCapacity_ == 0) ? config.packetCapacity_ : default_packet_capacity)
{
}


//=============================================================================
template <bcpp::message::protocol_concept P, bcpp::message::packet_concept T>
bool bcpp::message::transmitter<P, T>::send
(
    message_concept auto const & message
) requires (std::is_same_v<protocol, typename std::decay_t<decltype(message)>::protocol>)
{
    auto spaceRequired = message.size();
    if (auto spaceRemaining = (packet_.capacity() - packet_.size()); spaceRemaining >= spaceRequired)
    {
        packet_.resize(packet_.size() + spaceRequired);
        std::copy_n(reinterpret_cast<std::uint8_t const *>(&message), spaceRequired, packet_.end() - spaceRequired);
        return true;
    }

    flush();
    if (auto spaceRemaining = (packet_.capacity() - packet_.size()); spaceRemaining >= spaceRequired)
    {
        packet_.resize(packet_.size() + spaceRequired);
        std::copy_n(reinterpret_cast<std::uint8_t const *>(&message), spaceRequired, packet_.end() - spaceRequired);
        return true;
    }
    return false;
}


//=============================================================================
template <bcpp::message::protocol_concept P, bcpp::message::packet_concept T>
void bcpp::message::transmitter<P, T>::flush
(
)
{
    if (!packet_.empty())
        packetHandler_(*this, std::move(packet_));
    packet_ = packetAllocateHandler_(*this, packetCapacity_);
}
