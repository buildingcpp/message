#pragma once

#include "./protocol/protocol.h"

#include <concepts>
#include <tuple>
#include <type_traits>


namespace bcpp::message
{

    template <protocol_concept T0, typename T0::message_indicator>
    struct message;

    template <protocol_concept T0>
    struct message_header;

    template <typename T>
    concept message_concept = (std::is_same_v<T, message<typename T::protocol, T::type>> &&
        std::is_trivially_copyable_v<T> && std::is_base_of_v<message_header<typename T::protocol>, T>);

} // namespace bcpp::message


#include "./receiver/receiver.h"
#include "./transmitter/transmitter.h"
