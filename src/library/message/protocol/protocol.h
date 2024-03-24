#pragma once

#include "./protocol_traits.h"

#include <tuple>
#include <array>


namespace bcpp::message
{

    template <protocol_traits_concept T0, typename T0::message_indicator ... T1>
    struct protocol
    {
        using traits = T0;
        using message_indicator = typename T0::message_indicator;
        static auto constexpr message_arity = sizeof ... (T1);

        static constexpr std::array<message_indicator, message_arity> messageIndicators_{T1 ...};
        static constexpr message_indicator get(std::size_t index){return messageIndicators_[index];}
    };


    template <typename T>
    concept protocol_concept = std::is_same_v<T, 
            decltype([]<std::size_t ... N>(std::index_sequence<N ...>) -> protocol<typename T::traits, T::get(N) ...>
            {return {};}(std::make_index_sequence<T::messageIndicators_.size()>()))>;

} // namespace bcpp::message



//=============================================================================
static constexpr auto operator <=>
(
    bcpp::message::protocol_concept auto const & first,
    bcpp::message::protocol_concept auto const & second
) noexcept
{
    if constexpr (auto a = (first.traits() <=> second.traits()); a != 0)
        return a;
    return (first.messagesIndicators_ <=> second.messagesIndicators_);
}


//=============================================================================
static constexpr auto operator ==
(
    bcpp::message::protocol_concept auto const & first,
    bcpp::message::protocol_concept auto const & second
) noexcept
{
    if constexpr (auto a = (first.traits() == second.traits()); a != 0)
        return a;
    return (first.messagesIndicators_ == second.messagesIndicators_);
}
