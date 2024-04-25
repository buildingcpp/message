#pragma once

#include "./packet_queue.h"

#include <library/message/receiver.h>
#include <include/non_copyable.h>

#include <functional>
#include <span>
#include <vector>
#include <cstdint>
#include <queue>
#include <concepts>


namespace bcpp::message 
{

    template <protocol_concept T0, packet_queue_concept T1 = std::queue<std::vector<char const>>>
    class pipe final :
        non_copyable
    {
    public:

        using protocol = T0;
        using packet_queue = T1;
        using packet = typename packet_queue::value_type;

        struct configuration 
        {
        };

        using packet_discard_handler = std::function<void(pipe const &, packet &&)>;

        struct event_handlers
        {
            packet_discard_handler  packetDiscardHandler_;
        };

        template <typename ... Ts>
        pipe
        (
            configuration const &,
            event_handlers,
            Ts && ...
        );

        ~pipe();

        pipe
        (
            pipe &&
        );

        pipe & operator =
        (
            pipe &&
        );


        pipe & operator << 
        (
            packet &&
        );

        template <typename T_>
        bool process_next_message
        (
            receiver<T_, protocol> &
        );

        std::size_t get_bytes_available() const;

        bool empty() const;

        void close();

    private:

        void clear();

        bool buffer_next_packet();

        packet_queue            packets_;

        std::vector<char>       buffered_;

        packet_discard_handler  packetDiscardHandler_;

        std::size_t             bytesAvailable_{0};

        std::size_t             bytesConsumedInNextPacket_{0};

    };
    

    template <typename T>
    concept pipe_concept = std::is_same_v<T, pipe<typename T::protocol, typename T::packet_queue>>;

} // namespace bcpp::message


//=============================================================================
template <bcpp::message::protocol_concept T0, bcpp::message::packet_queue_concept T1>
template <typename ... Ts>
bcpp::message::pipe<T0, T1>::pipe 
(
    configuration const & config,
    event_handlers eventHandlers,
    Ts && ... packetQueueArgs
):
    packets_(std::forward<Ts>(packetQueueArgs) ...),
    packetDiscardHandler_(eventHandlers.packetDiscardHandler_)
{
}


//=============================================================================
template <bcpp::message::protocol_concept T0, bcpp::message::packet_queue_concept T1>
bcpp::message::pipe<T0, T1>::~pipe
(
)
{
    clear();
}


//=============================================================================
template <bcpp::message::protocol_concept T0, bcpp::message::packet_queue_concept T1>
bcpp::message::pipe<T0, T1>::pipe
(
    pipe && other
):
    packets_(std::move(other.packets_)),
    buffered_(std::move(other.buffered_)),
    packetDiscardHandler_(std::move(other.packetDiscardHandler_)),
    bytesAvailable_(other.bytesAvailable_),
    bytesConsumedInNextPacket_(other.bytesConsumedInNextPacket_)
{
    other.packetDiscardHandler_ = nullptr;
    other.bytesAvailable_ = 0;
    other.bytesConsumedInNextPacket_ = 0;
}

        
//=============================================================================
template <bcpp::message::protocol_concept T0, bcpp::message::packet_queue_concept T1>
auto bcpp::message::pipe<T0, T1>::operator =
(
    pipe && other
) -> pipe & 
{
    if (this != & other)
    {
        clear();
        packets_ = std::move(other.packets_);
        buffered_ = std::move(other.buffered_);
        packetDiscardHandler_ = std::move(other.packetDiscardHandler_);
        bytesAvailable_ = other.bytesAvailable_;
        bytesConsumedInNextPacket_ = other.bytesConsumedInNextPacket_;
        other.packetDiscardHandler_ = nullptr;
        other.bytesAvailable_ = 0;
        other.bytesConsumedInNextPacket_ = 0;
    }
    return *this;
}


//=============================================================================
template <bcpp::message::protocol_concept T0, bcpp::message::packet_queue_concept T1>
void bcpp::message::pipe<T0, T1>::clear
(
)
{
    while (!packets_.empty())
    {
        auto && p = packets_.front();
        bytesAvailable_ -= p.size();
        if (packetDiscardHandler_)
            packetDiscardHandler_(*this, std::forward<packet>(p));
        packets_.pop();
    }

    buffered_.clear();
    bytesAvailable_ = 0;
    bytesConsumedInNextPacket_ = 0;
}


//=============================================================================
template <bcpp::message::protocol_concept T0, bcpp::message::packet_queue_concept T1>
auto bcpp::message::pipe<T0, T1>::operator << 
(
    packet && p
) -> pipe &
{
    bytesAvailable_ += p.size();
    packets_.push(std::move(p));
    return *this;
}


//=============================================================================
template <bcpp::message::protocol_concept T0, bcpp::message::packet_queue_concept T1>
bool bcpp::message::pipe<T0, T1>::buffer_next_packet
(
)
{
    if (packets_.empty())
        return false;
    auto && p = packets_.front();
    std::copy_n(p.begin() + bytesConsumedInNextPacket_, p.size() - bytesConsumedInNextPacket_, std::back_inserter(buffered_));
    if (packetDiscardHandler_)
        packetDiscardHandler_(*this, std::forward<packet>(p));
    packets_.pop();
    bytesConsumedInNextPacket_ = 0;
    return true;
}


//=============================================================================
template <bcpp::message::protocol_concept T0, bcpp::message::packet_queue_concept T1>
template <typename T_>
bool bcpp::message::pipe<T0, T1>::process_next_message
(
    receiver<T_, protocol> & messageReceiver
)
{
    using message_header = message_header<protocol>;
    static auto constexpr minimum_data_to_parse_header = sizeof(message_header);

    if (bytesAvailable_ < minimum_data_to_parse_header)
        return false; // insufficient data to continue

    while (buffered_.empty())
    {
        // attempt to parse directly from the next packet
        auto & nextPacket = packets_.front();
        auto bytesAvailableInNextPacket = nextPacket.size() - bytesConsumedInNextPacket_;
        if (bytesAvailableInNextPacket < minimum_data_to_parse_header)
        {
            // the next packet isn't large enough to represent the header so buffer it
            buffer_next_packet();
            continue;
        }

        // there is sufficient data in the next packet to represent a header
        auto const & messageHeader = *reinterpret_cast<message_header const *>(nextPacket.data() + bytesConsumedInNextPacket_);
        auto messageSize = messageHeader.size();
        if (bytesAvailableInNextPacket < messageSize)
        {
            // the next packet isn't large enough to represent the entire message so buffer it
            buffer_next_packet();
            continue;
        }

        // the next packet has sufficient data to represent an entire message
        bytesConsumedInNextPacket_ += messageSize;
        bytesAvailable_ -= messageSize;
        messageReceiver.process(std::span(reinterpret_cast<std::uint8_t const *>(&messageHeader), messageSize)); // dispatch the message
        return true; // message dispatched
    }

    while (true)
    {
        // parse using the buffered data
        if (buffered_.size() < minimum_data_to_parse_header)
        {
            // buffered data contains insufficient data to represent the message header so buffer next packet
            if (!buffer_next_packet())
                return false; // insufficient data to represent a message at this time
            continue;
        }
        // there is sufficient data in the next packet to represent a header
        auto const & messageHeader = *reinterpret_cast<message_header const *>(buffered_.data());
        auto messageSize = messageHeader.size();
        if (buffered_.size() < messageSize)
        {
            // the next packet isn't large enough to represent the entire message so buffer more
            if (!buffer_next_packet())
                return false; // insufficient data to represent a header at this time
            continue;
        }

        // the next packet has sufficient data to represent an entire message
        bytesAvailable_ -= messageSize;
        messageReceiver.process(std::span(reinterpret_cast<std::uint8_t const *>(buffered_.data()), messageSize)); // dispatch the message
        buffered_.erase(buffered_.begin(), buffered_.begin() + messageSize);
        return true; // message dispatched
    }
}


//=============================================================================
template <bcpp::message::protocol_concept T0, bcpp::message::packet_queue_concept T1>
std::size_t bcpp::message::pipe<T0, T1>::get_bytes_available
(
) const
{
    return bytesAvailable_;
}


//=============================================================================
template <bcpp::message::protocol_concept T0, bcpp::message::packet_queue_concept T1>
bool bcpp::message::pipe<T0, T1>::empty
(
) const
{
    return (bytesAvailable_ == 0);
}
