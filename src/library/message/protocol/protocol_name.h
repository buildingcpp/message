#pragma once

#include <include/constexpr_string.h>

#include <concepts>
#include <type_traits>


namespace bcpp::message
{

    template <std::size_t N>
    struct protocol_name : 
        public constexpr_string<N>
    {
        constexpr protocol_name
        (
            char const (&value)[N]
        ) noexcept :
            constexpr_string<N>(value)
        {
        }
    };


    template <typename T>
    concept protocol_name_concept = std::is_same_v<T, protocol_name<T::size()>>;

} // namespace bcpp::message


//=============================================================================
static constexpr auto operator <=>
(
    bcpp::message::protocol_name_concept auto const & first,
    bcpp::message::protocol_name_concept auto const & second
) noexcept
{
    return (std::string_view(first) <=> std::string_view(second));
}


//=============================================================================
static constexpr auto operator ==
(
    bcpp::message::protocol_name_concept auto const & first,
    bcpp::message::protocol_name_concept auto const & second
) noexcept
{
    return (std::string_view(first) == std::string_view(second));
}
