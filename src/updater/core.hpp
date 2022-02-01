#pragma once

#include <pools/fiber_pool.hpp>


template <typename traits>
class core
{
public:
    core(uint16_t number_of_threads) noexcept;

    template <typename T>
    void start(T&& main_loop) noexcept;

private:
    np::fiber_pool<traits> _fiber_pool;
    uint16_t _number_of_threads;
};


template <typename traits>
core<traits>::core(uint16_t number_of_threads) :
    _fiber_pool(),
    _number_of_threads(number_of_threads)
{}


template <typename traits>
template <typename T>
void core<traits>::start(T&& main_loop) noexcept
{
    _fiber_pool.push(std::forward<T>(main_loop));
    _fiber_pool.start(number_of_threads);
    // This point is only reached after the pool is stopped
}
