#pragma once

#include <pool/fiber_pool.hpp>
#include "storage/ticket.hpp"

#include <vector>


template <uint32_t inplace_function_size>
class task_manager
{
    using tasks = std::vector<stdext::inplace_function<void(), inplace_function_size>>;

public:
    template <typename F>
    void schedule(F&& function) noexcept;

    template <typename F, typename... Args>
    void schedule_if(F&& function, Args&&... tickets) noexcept;

    void execute() noexcept;
};

template <uint32_t inplace_function_size>
template <typename F>
void task_manager<inplace_function_size>::schedule(F&& function) noexcept
{
    auto& local_tasks = np::this_fiber::template threadlocal<tasks>();
    local_tasks.push_back(std::forward<F>(function));
}

template <uint32_t inplace_function_size>
template <typename F, typename... Args>
void task_manager<inplace_function_size>::schedule_if(F&& function, Args&&... tickets) noexcept
{
    schedule([function = std::move(function), tickets...]() mutable {
        if ((tickets->valid() && ...))
        {
            function(tickets->get()->derived()...);
        }
    });
}

template <uint32_t inplace_function_size>
void task_manager<inplace_function_size>::execute() noexcept
{
    auto fiber_pool = np::this_fiber::fiber_pool();
    auto*& per_thread_tasks = fiber_pool->template threadlocal_all<tasks>();
    for (auto i = 0, size = fiber_pool->maximum_worker_id(); i < size; ++i)
    {
        for (auto&& task : per_thread_tasks[i])
        {
            task();
        }

        per_thread_tasks[i].clear();
    }
}
