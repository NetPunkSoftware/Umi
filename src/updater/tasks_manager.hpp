#pragma once

#include <pool/fiber_pool.hpp>
#include "storage/ticket.hpp"

#include <vector>


template <uint32_t inplace_function_size>
class task_manager
{
    class dual_vector_scheduler
    {
        using vector_t = std::vector<stdext::inplace_function<void(), inplace_function_size>>;

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
    auto& local_tasks = np::this_fiber::template threadlocal<dual_vector_scheduler>();
    local_tasks.current().push_back(std::forward<F>(function));
}

template <uint32_t inplace_function_size>
template <typename F, typename... Args>
void task_manager<inplace_function_size>::schedule_if(F&& function, Args&&... tickets) noexcept
{
    schedule([function = std::move(function), tickets...]() mutable {
        if ((tickets->valid() && ...))
        {
            function(tickets->get()...);
        }
    });
}

template <uint32_t inplace_function_size>
void task_manager<inplace_function_size>::execute() noexcept
{
    auto fiber_pool = np::this_fiber::fiber_pool();
    auto& per_thread_tasks = fiber_pool->template threadlocal_all<dual_vector_scheduler>();
    for (uint8_t i = 0, size = fiber_pool->maximum_worker_id(); i < size; ++i)
    {
        auto& iter_safe_buffer = per_thread_tasks[i].swap();
        for (auto&& task : iter_safe_buffer)
        {
            task();
        }

        iter_safe_buffer.clear();
    }
}
