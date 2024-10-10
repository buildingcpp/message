#pragma once

#include "./packet.h"


namespace bcpp::message
{

    template <typename T>
    concept packet_queue_concept = requires (T queue, typename T::value_type p)
            {
                requires (packet_concept<typename T::value_type>); 
                queue.push(p); 
                queue.pop(); 
                queue.empty(); 
                queue.size(); 
                queue.front();
            };

}
