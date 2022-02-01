#pragma once

#include <atomic>
#include <inttypes.h>


#ifdef _WIN32
    #define conditional_constexpr 
#else
    #define conditional_constexpr constexpr
#endif

class generator
{
public:
    constexpr generator() noexcept;

    inline conditional_constexpr uint64_t peek() const noexcept;
    inline conditional_constexpr uint64_t next() noexcept;

private:
    // Current next id
    std::atomic<uint64_t> _current;
};


constexpr generator::generator() noexcept:
    _current(0)
{}

inline conditional_constexpr uint64_t generator::peek() const noexcept
{
    return _current;
}

inline conditional_constexpr uint64_t generator::next() noexcept
{
    return _current++;
}
