#pragma once

#include <pool/fiber_pool.hpp>
#include "storage/ticket.hpp"

#include <variant>
#include <vector>


template <typename... Ts>
class variant_task_manager
{
    class dual_vector_scheduler
    {
        using vector_t = std::vector<std::variant<Ts...>>;

    public:
        dual_vector_scheduler() noexcept :
            _vector_1(),
            _vector_2()
        {
            _current_vector = &_vector_1;
        }

        inline vector_t& current() noexcept
        {
            return *_current_vector;
        }

        inline vector_t& swap() noexcept
        {
            vector_t* old = _current_vector;
            if (_current_vector == &_vector_1)
            {
                _current_vector = &_vector_2;
            }
            else
            {
                _current_vector = &_vector_1;
            }
            return *old;
        }

    private:
        vector_t* _current_vector;
        vector_t _vector_1;
        vector_t _vector_2;
    };

public:
    template <typename T>
    void submit(T&& value) noexcept;

    template <typename V>
    void execute(V&& visitor) noexcept;
};

template <typename... Ts>
template <typename T>
void variant_task_manager<Ts...>::submit(T&& value) noexcept
{
    auto& local_tasks = np::this_fiber::template threadlocal<dual_vector_scheduler>();
    local_tasks.current().push_back(std::forward<T>(value));
}

template <typename... Ts>
template <typename V>
void variant_task_manager<Ts...>::execute(V&& visitor) noexcept
{
    auto fiber_pool = np::this_fiber::fiber_pool();
    auto& per_thread_tasks = fiber_pool->template threadlocal_all<dual_vector_scheduler>();
    for (uint8_t i = 0, size = fiber_pool->maximum_worker_id(); i < size; ++i)
    {
        auto& iter_safe_buffer = per_thread_tasks[i].swap();
        for (auto&& variant : iter_safe_buffer)
        {
            std::visit(visitor, variant);
        }

        iter_safe_buffer.clear();
    }
}
