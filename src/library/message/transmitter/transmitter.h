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

        template <message_concept M, typename ... Ts>
        bool emplace
        (
            Ts && ...
        ) requires (std::is_same_v<protocol, typename M::protocol>);

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
template <bcpp::message::message_concept M, typename ... Ts>
bool bcpp::message::transmitter<P, T>::emplace
(
    // use placement new if possible
    Ts && ... args
) requires (std::is_same_v<protocol, typename M::protocol>)
{
    if constexpr (requires (){M::size(args ...);} || requires(){M::size();})
    {
        // emplace requires that the message's size() function be static and accepts the provided arguments.
        // This basiclly means that to embed a message, its size must be calculable without requiring 
        // consturction of the message.  Anything else defeats the point of an optimized emplace call entirely.
        auto spaceRequired = 0;
        if constexpr (requires (){M::size(args ...);})
            spaceRequired = M::size(args ...); // has a ctor that takes the provided args
        else
            spaceRequired = M::size(); // only has the option to use the default ctor

        if (auto spaceRemaining = (packet_.capacity() - packet_.size()); spaceRemaining >= spaceRequired)
        {
            packet_.resize(packet_.size() + spaceRequired);
            new (&*packet_.end() - spaceRequired) M(std::forward<Ts>(args) ...);
            return true;
        }

        flush();
        if (auto spaceRemaining = (packet_.capacity() - packet_.size()); spaceRemaining >= spaceRequired)
        {
            packet_.resize(packet_.size() + spaceRequired);
            new (&*packet_.end() - spaceRequired) M(std::forward<Ts>(args) ...);
            return true;
        }
        return false;
    }
    else
    {
        // dynamic message can not be constructed in place because we don't know how much space to reserve
        // or require in the current packet.  Instead, we construct the message and then send via copy.
        return send(M(std::forward<Ts>(args) ...));
    }
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
