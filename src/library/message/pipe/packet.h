#pragma once

#include <library/message/message.h>
#include <include/non_copyable.h>

#include <cstring>


namespace bcpp::message
{

    template <protocol_concept T0, typename T1>
    class packet :
        non_copyable
    {
    public:

        using protocol = T0;
        using value_type = T1;
        using iterator = typename T1::iterator;
        using const_iterator = typename T1::const_iterator;

        static_assert(requires (T1 packet)
            {
                packet.begin(); 
                packet.end();
                packet.data(); 
                packet.size(); 
                packet.empty(); 
                packet.end();
            });

        template <typename ... Ts>
        packet
        (
            Ts && ...
        );

        ~packet();

        packet
        (
            packet &&
        ) noexcept;

        packet & operator =
        (
            packet &&
        ) noexcept;

        bool insert
        (
            message_concept auto const & message
        ) requires (std::is_same_v<protocol, typename std::decay_t<decltype(message)>::protocol>);

        auto begin() const{return value_.begin();}
        auto begin() {return value_.begin();}
        auto end() const{return end_;}
        auto end() {return end_;}
        auto size() const {return (end_ - value_.begin());}
        auto empty() const {return (value_.begin() == end_);}
        auto capacity() const{return value_.capacity();}
        auto data() const{return value_.data();}
        auto data() {return value_.data();}

    private:

        value_type  value_;

        iterator    end_;

    }; // class packet


    template <typename T>
    concept packet_concept = std::is_same_v<T, packet<typename T::protocol, typename T::value_type>>;

} // namespace bcpp::message


//=============================================================================
template <bcpp::message::protocol_concept T0, typename T1>
template <typename ... Ts>
bcpp::message::packet<T0, T1>::packet 
(
    Ts && ... args
):
    value_(std::forward<Ts>(args) ...),
    end_(value_.begin())
{
}


//=============================================================================
template <bcpp::message::protocol_concept T0, typename T1>
bcpp::message::packet<T0, T1>::~packet
(
)
{
}


//=============================================================================
template <bcpp::message::protocol_concept T0, typename T1>
bcpp::message::packet<T0, T1>::packet
(
    packet && other
) noexcept
{
    auto size = other.size();
    value_ = std::move(other.value_);
    end_ = value_.begin() + size;
}


//=============================================================================
template <bcpp::message::protocol_concept T0, typename T1>
auto bcpp::message::packet<T0, T1>::operator =
(
    packet && other
) noexcept -> packet &
{
    if (this != &other)
    {
        auto size = other.size();
        value_ = std::move(other.value_);
        end_ = value_.begin() + size;
    }
    return *this;
}


//=============================================================================
template <bcpp::message::protocol_concept T0, typename T1>
bool bcpp::message::packet<T0, T1>::insert
(
    message_concept auto const & message
) 
requires (std::is_same_v<protocol, typename std::decay_t<decltype(message)>::protocol>)
{
    auto size = message.size();
    if ((end_ + size) > value_.end())
        return false;
    std::memcpy(&*end_, &message, size);
    end_ += size;
    return true;
}

