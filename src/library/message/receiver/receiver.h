#pragma once

#include <library/message/transport/packet_queue.h>

#include <include/non_copyable.h>

#include <span>
#include <type_traits>
#include <cstdint>
#include <functional>
#include <vector>
#include <queue>
#include <concepts>


namespace bcpp::message
{

    template <typename T, protocol_concept P, packet_queue_concept Q = std::queue<std::vector<char const>>>
    class receiver :
        non_copyable
    {
    public:

        using target = T;
        using protocol = P;
        using packet_queue = Q;
        using packet = typename packet_queue::value_type;
        using protocol_traits = typename protocol::traits;
        using message_indicator = protocol_traits::message_indicator;
        using underlying_message_indicator = std::make_unsigned_t<std::underlying_type_t<message_indicator>>;

        struct configuration 
        {
        };

        using packet_discard_handler = std::function<void(receiver const &, packet &&)>;

        struct event_handlers
        {
            packet_discard_handler  packetDiscardHandler_;
        };

        template <typename ... Ts>
        receiver
        (
            configuration const &,
            event_handlers,
            Ts && ...
        );

        receiver
        (
            receiver && other
        );
              
        receiver & operator =
        (
            receiver && other
        );
        
        ~receiver();

        bool process_next_message();

        std::size_t get_bytes_available() const;

        bool empty() const;

        void close();

        receiver & operator << 
        (
            packet &&
        );

    protected:

        void process
        (
            std::span<std::uint8_t const> source
        )
        {
            using message_header = bcpp::message::message_header<P>;
            message_header const & messageHeader = *reinterpret_cast<message_header const *>(source.data());
            if (auto callback = callback_[static_cast<underlying_message_indicator>(messageHeader.get_message_indicator())]; callback != nullptr)
                callback(*this, source.data());
        }

    private:

        static auto constexpr bits_per_byte = 8;
        static auto constexpr max_underlying_message_indicator_value = (1 << (sizeof(underlying_message_indicator) * bits_per_byte));

        template <message_indicator M>
        static target & dispatch_message
        (
            receiver & self,
            void const * address
        )
        {
            using message_type = message<protocol, M>;
            return (reinterpret_cast<target &>(self)(self, *reinterpret_cast<message_type const *>(address)));
        }

        void clear();

        bool buffer_next_packet();

        packet_queue            packets_;

        std::vector<char>       buffered_;

        packet_discard_handler  packetDiscardHandler_;

        std::size_t             bytesAvailable_{0};

        std::size_t             bytesConsumedInNextPacket_{0};

        static std::array<target &(*)(receiver &, void const *), max_underlying_message_indicator_value> callback_;

    }; // class receiver


    template <typename T, protocol_concept P, packet_queue_concept Q>
    std::array<T &(*)(receiver<T, P, Q> &, void const *), receiver<T, P, Q>::max_underlying_message_indicator_value> receiver<T, P, Q>::callback_;


    template <typename T>
    concept receiver_concept = std::is_same_v<T, receiver<typename T::target, typename T::protocol, typename T::packet_queue>>;

} // namespace bcpp::message


//=============================================================================
template <typename T, bcpp::message::protocol_concept P, bcpp::message::packet_queue_concept Q>
template <typename ... Ts>
bcpp::message::receiver<T, P, Q>::receiver 
(
    configuration const & config,
    event_handlers eventHandlers,
    Ts && ... packetQueueArgs
):
    packets_(std::forward<Ts>(packetQueueArgs) ...),
    packetDiscardHandler_(eventHandlers.packetDiscardHandler_)
{    
    static auto once = [&]<std::size_t ... N>(std::index_sequence<N ...>)
    {
        for (auto & callback : callback_)
            callback = nullptr;
        ([&]()
            {    
                // only configure a callback if 'target' supports receiving that message type
                if constexpr (requires (target t, receiver d, message<protocol, P::get(N)> m){t(d, m);})
                    callback_[static_cast<underlying_message_indicator>(P::get(N))] = dispatch_message<P::get(N)>;
                else
                {
                    // TODO: add some kind of warning that this type of receiver has no handler for this type of message
                }
            }(), ...);
        return true;
    }(std::make_index_sequence<protocol::messageIndicators_.size()>());
}


//=============================================================================
template <typename T, bcpp::message::protocol_concept P, bcpp::message::packet_queue_concept Q>
bcpp::message::receiver<T, P, Q>::~receiver
(
)
{
    clear();
}


//=============================================================================
template <typename T, bcpp::message::protocol_concept P, bcpp::message::packet_queue_concept Q>
bcpp::message::receiver<T, P, Q>::receiver
(
    receiver && other
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
template <typename T, bcpp::message::protocol_concept P, bcpp::message::packet_queue_concept Q>
auto bcpp::message::receiver<T, P, Q>::operator =
(
    receiver && other
) -> receiver & 
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
template <typename T, bcpp::message::protocol_concept P, bcpp::message::packet_queue_concept Q>
void bcpp::message::receiver<T, P, Q>::clear
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
template <typename T, bcpp::message::protocol_concept P, bcpp::message::packet_queue_concept Q>
auto bcpp::message::receiver<T, P, Q>::operator << 
(
    packet && p
) -> receiver &
{
    bytesAvailable_ += p.size();
    packets_.push(std::move(p));
    return *this;
}


//=============================================================================
template <typename T, bcpp::message::protocol_concept P, bcpp::message::packet_queue_concept Q>
bool bcpp::message::receiver<T, P, Q>::buffer_next_packet
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
template <typename T, bcpp::message::protocol_concept P, bcpp::message::packet_queue_concept Q>
bool bcpp::message::receiver<T, P, Q>::process_next_message
(
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
        reinterpret_cast<target &>(*this).process(std::span(reinterpret_cast<std::uint8_t const *>(&messageHeader), messageSize)); // dispatch the message
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
        reinterpret_cast<target &>(*this).process(std::span(reinterpret_cast<std::uint8_t const *>(buffered_.data()), messageSize)); // dispatch the message
        buffered_.erase(buffered_.begin(), buffered_.begin() + messageSize);
        return true; // message dispatched
    }
}


//=============================================================================
template <typename T, bcpp::message::protocol_concept P, bcpp::message::packet_queue_concept Q>
std::size_t bcpp::message::receiver<T, P, Q>::get_bytes_available
(
) const
{
    return bytesAvailable_;
}


//=============================================================================
template <typename T, bcpp::message::protocol_concept P, bcpp::message::packet_queue_concept Q>
bool bcpp::message::receiver<T, P, Q>::empty
(
) const
{
    return (bytesAvailable_ == 0);
}
