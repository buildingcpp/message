#pragma once

#include "./protocol.h"


#include <concepts>
#include <tuple>
#include <type_traits>


namespace bcpp::message
{


    template <protocol_concept T0, typename T0::message_indicator>
    struct message;


    template <protocol_concept T0>
    struct message_header;


    template <protocol_concept T0, typename T0::message_indicator>
    struct message_footer;


} // namespace bcpp::message
