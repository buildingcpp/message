#pragma once

#include "./protocol/protocol.h"

#include <span>
#include <type_traits>
#include <cstdint>


namespace bcpp::message
{

    template <typename T, protocol_concept P>
    class receiver 
    {
    public:

        using target = T;
        using protocol = P;
        using protocol_traits = typename protocol::traits;
        using message_indicator = protocol_traits::message_indicator;
        using underlying_message_indicator = std::make_unsigned_t<std::underlying_type_t<message_indicator>>;

        receiver();

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

        static std::array<target &(*)(receiver &, void const *), max_underlying_message_indicator_value> callback_;

    }; // class receiver


    template <typename T, protocol_concept P>
    std::array<T &(*)(receiver<T, P> &, void const *), receiver<T, P>::max_underlying_message_indicator_value> receiver<T, P>::callback_;


    template <typename T>
    concept receiver_concept = std::is_same_v<T, receiver<typename T::target, typename T::protocol>>;

} // namespace bcpp::message


//=============================================================================
template <typename T, bcpp::message::protocol_concept P>
bcpp::message::receiver<T, P>::receiver
(
)
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
            }(), ...);
        return true;
    }(std::make_index_sequence<protocol::messageIndicators_.size()>());
}
