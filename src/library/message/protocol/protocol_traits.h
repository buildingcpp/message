#pragma once

#include "./protocol_name.h"
#include "./version.h"

#include <concepts>
#include <type_traits>


namespace bcpp::message
{

    template <protocol_name T0, version V, typename T1>
    struct protocol_traits
    {
        using message_indicator = T1;
        static constexpr auto name_{T0};
        static constexpr auto version_{V};
    };
    

    template <typename T>
    concept protocol_traits_concept = std::is_same_v<T, protocol_traits<T::name_, T::version_, typename T::message_indicator>>;
    
} // namespace bcpp::message


//=============================================================================
static constexpr auto operator <=>
(
    bcpp::message::protocol_traits_concept auto const & first,
    bcpp::message::protocol_traits_concept auto const & second
) noexcept
{
    if constexpr (auto a = (first.name_ <=> second.name_); a != 0)
        return a;
    return (first.version_ <=> second.version_);
}


//=============================================================================
static constexpr auto operator ==
(
    bcpp::message::protocol_traits_concept auto const & first,
    bcpp::message::protocol_traits_concept auto const & second
) noexcept
{
    if constexpr (first.name_ != second.name_)
        return false;
    return (first.version_ == second.version_);
}

