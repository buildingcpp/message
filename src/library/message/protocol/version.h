#pragma once

#include <cstdint>


namespace bcpp
{

    struct version
    {

        constexpr version
        (
            std::int32_t major,
            std::int32_t minor,
            char letter
        ) noexcept :
            major_(major),
            minor_(minor),
            letter_(letter)
        {
        }

        auto constexpr operator <=> (version const &) const noexcept = default;

        std::int32_t const major_;
        std::int32_t const minor_;
        char const letter_;
    };

} // namespace bcpp