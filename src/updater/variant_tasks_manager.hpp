#pragma once

#include <pool/fiber_pool.hpp>
#include "storage/ticket.hpp"

#include <variant>
#include <vector>


template <typename... Ts>
class task_manager
{
    using tasks = std::vector<std::variant<Ts...>>;

public:
    template <typename T>
    void schedule(T&& value) noexcept;

    template <typename V>
    void execute(V&& visitor) noexcept;
};

template <typename... Ts>
template <typename T>
void task_manager<Ts...>::schedule(T&& value) noexcept
{
    auto& local_tasks = this_fiber::fiber_pool()->template threadlocal<tasks>();
    local_tasks.push_back(std::forward<T>(value));
}

template <typename... Ts>
template <typename V>
void task_manager<Ts...>::execute(V&& visitor) noexcept
{
    auto fiber_pool = this_fiber::fiber_pool();
    auto*& per_thread_tasks = fiber_pool->template threadlocal_all<tasks>();
    for (auto i = 0, size = fiber_pool->maximum_worker_id(); i < size; ++i)
    {
        for (auto&& variant : per_thread_tasks[i])
        {
            visitor(variant);
        }

        per_thread_tasks[i].clear();
    }
}
