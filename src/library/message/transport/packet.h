#pragma once


namespace bcpp::message
{

    template <typename T>
    concept packet_concept = requires (T packet)
            {
                requires (sizeof(*packet.data()) == 1);
                packet.begin();
                packet.end();
                packet.size();
                packet.empty();
                packet.capacity();
                packet.data();
            };

} // namespace bcpp::message
