#pragma once

#include <synchronization/counter.hpp>
#include <pool/fiber_pool.hpp>


struct scheme_view
{
    template <typename traits, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous(np::counter& counter, np::fiber_pool<traits>* pool, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use continuous_by when the scheme contains mixed layouts"
        );
        
        if (scheme.size() == 0)
        {
            return;
        }

        pool->push([&scheme, callback = std::move(callback)] ()
        {
            for (auto combined : ::ranges::views::zip(scheme.template get<types>().range()...))
            {
                std::apply(callback, combined);
            }
        }, counter);
        
#if !defined(NDEBUG)
        counter.on_wait_done([&scheme]() {
            (..., scheme.template get<types>().unlock_writes());
        });
#endif
    }

    template <typename By, typename traits, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous_by(np::counter& counter, np::fiber_pool<traits>* pool, S<types...>& scheme, C&& callback) noexcept
    {
        if (scheme.size() == 0)
        {
            return;
        }

        pool->push([&scheme, callback = std::move(callback)]()
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range())
            {
                std::apply(callback, scheme.search(obj->id()));
            }
        }, counter);

#if !defined(NDEBUG)
        counter.on_wait_done([&scheme]() {
            (..., scheme.template get<types>().unlock_writes());
            });
#endif
    }

    template <typename traits, template <typename...> class S, typename C, typename... types>
    inline static constexpr void parallel(np::counter& counter, np::fiber_pool<traits>* pool, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use parallel_by when the scheme contains mixed layouts"
            );

        if (scheme.size() == 0)
        {
            return;
        }

        for (auto combined : ::ranges::views::zip(scheme.template get<types>().range()...))
        {
            pool->push([&scheme, combined, callback = std::move(callback)]() mutable
            {
                std::apply(callback, combined);
            }, counter);
        }

#if !defined(NDEBUG)
        counter.on_wait_done([&scheme]() {
            (..., scheme.template get<types>().unlock_writes());
            });
#endif
    }

    template <typename By, typename traits, template <typename...> class S, typename O, typename C, typename... types>
    inline static constexpr void parallel_by(np::counter& counter, np::fiber_pool<traits>* pool, S<types...>& scheme, C&& callback) noexcept
    {
        if (scheme.size() == 0)
        {
            return;
        }

        auto& component = scheme.template get<By>();
        for (auto obj : component.range())
        {
            pool->push([&scheme, id = obj->id(), callback = std::move(callback)]() mutable
            {
                std::apply(callback, scheme.search(id));
            }, counter);
        }

#if !defined(NDEBUG)
        counter.on_wait_done([&scheme]() {
            (..., scheme.template get<types>().unlock_writes());
            });
#endif
    }

private:
    scheme_view();
};

struct scheme_view_until_partition
{
    template <typename traits, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous(np::counter& counter, np::fiber_pool<traits>* pool, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use continuous_by when the scheme contains mixed layouts"
            );

        if (scheme.size_until_partition() == 0)
        {
            return;
        }

        pool->push([&scheme, callback = std::move(callback)]()
        {
            for (auto combined : ::ranges::views::zip(scheme.template get<types>().range_until_partition()...))
            {
                std::apply(callback, combined);
            }
        }, counter);

#if !defined(NDEBUG)
        counter.on_wait_done([&scheme]() {
            (..., scheme.template get<types>().unlock_writes());
            });
#endif
    }

    template <typename By, typename traits, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous_by(np::counter& counter, np::fiber_pool<traits>* pool, S<types...>& scheme, C&& callback) noexcept
    {
        if (scheme.size_until_partition() == 0)
        {
            return;
        }

        pool->push([&scheme, callback = std::move(callback)]()
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range_until_partition())
            {
                std::apply(callback, scheme.search(obj->id()));
            }
        }, counter);

#if !defined(NDEBUG)
        counter.on_wait_done([&scheme]() {
            (..., scheme.template get<types>().unlock_writes());
            });
#endif
    }

    template <typename traits, template <typename...> class S, typename C, typename... types>
    inline static constexpr void parallel(np::counter& counter, np::fiber_pool<traits>* pool, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use parallel_by when the scheme contains mixed layouts"
            );

        if (scheme.size_until_partition() == 0)
        {
            return;
        }

        for (auto combined : ::ranges::views::zip(scheme.template get<types>().range_until_partition()...))
        {
            pool->push([&scheme, combined, callback = std::move(callback)]() mutable
            {
                std::apply(callback, combined);
            }, counter);
        }

#if !defined(NDEBUG)
        counter.on_wait_done([&scheme]() {
            (..., scheme.template get<types>().unlock_writes());
            });
#endif
    }

    template <typename By, typename traits, template <typename...> class S, typename O, typename C, typename... types>
    inline static constexpr void parallel_by(np::counter& counter, np::fiber_pool<traits>* pool, S<types...>& scheme, C&& callback) noexcept
    {
        if (scheme.size_until_partition() == 0)
        {
            return;
        }

        auto& component = scheme.template get<By>();
        for (auto obj : component.range_until_partition())
        {
            pool->push([&scheme, id = obj->id(), callback = std::move(callback)]() mutable
            {
                std::apply(callback, scheme.search(id));
            }, counter);
        }

#if !defined(NDEBUG)
        counter.on_wait_done([&scheme]() {
            (..., scheme.template get<types>().unlock_writes());
            });
#endif
    }

private:
    scheme_view_until_partition();
};

struct scheme_view_from_partition
{
    template <typename traits, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous(np::counter& counter, np::fiber_pool<traits>* pool, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use continuous_by when the scheme contains mixed layouts"
            );

        if (scheme.size_from_partition() == 0)
        {
            return;
        }

        pool->push([&scheme, callback = std::move(callback)]()
        {
            for (auto combined : ::ranges::views::zip(scheme.template get<types>().range_from_partition()...))
            {
                std::apply(callback, combined);
            }
        }, counter);

#if !defined(NDEBUG)
        counter.on_wait_done([&scheme]() {
            (..., scheme.template get<types>().unlock_writes());
            });
#endif
    }

    template <typename By, typename traits, template <typename...> class S, typename C, typename... types>
    inline static constexpr void continuous_by(np::counter& counter, np::fiber_pool<traits>* pool, S<types...>& scheme, C&& callback) noexcept
    {
        if (scheme.size_from_partition() == 0)
        {
            return;
        }

        pool->push([&scheme, callback = std::move(callback)]()
        {
            auto& component = scheme.template get<By>();
            for (auto obj : component.range_from_partition())
            {
                std::apply(callback, scheme.search(obj->id()));
            }
        }, counter);

#if !defined(NDEBUG)
        counter.on_wait_done([&scheme]() {
            (..., scheme.template get<types>().unlock_writes());
            });
#endif
    }

    template <typename traits, template <typename...> class S, typename C, typename... types>
    inline static constexpr void parallel(np::counter& counter, np::fiber_pool<traits>* pool, S<types...>& scheme, C&& callback) noexcept
    {
        static_assert(
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::continuous) && ...) ||
            (has_storage_tag(types::tag, storage_grow::none, storage_layout::partitioned) && ...),
            "Use parallel_by when the scheme contains mixed layouts"
            );

        if (scheme.size_from_partition() == 0)
        {
            return;
        }

        for (auto combined : ::ranges::views::zip(scheme.template get<types>().range_from_partition()...))
        {
            pool->push([&scheme, combined, callback = std::move(callback)]() mutable
            {
                std::apply(callback, combined);
            }, counter);
        }

#if !defined(NDEBUG)
        counter.on_wait_done([&scheme]() {
            (..., scheme.template get<types>().unlock_writes());
            });
#endif
    }

    template <typename By, typename traits, template <typename...> class S, typename O, typename C, typename... types>
    inline static constexpr void parallel_by(np::counter& counter, np::fiber_pool<traits>* pool, S<types...>& scheme, C&& callback) noexcept
    {
        if (scheme.size_from_partition() == 0)
        {
            return;
        }

        auto& component = scheme.template get<By>();
        for (auto obj : component.range_from_partition())
        {
            pool->push([&scheme, id = obj->id(), callback = std::move(callback)]() mutable
            {
                std::apply(callback, scheme.search(id));
            }, counter);
        }
        
#if !defined(NDEBUG)
        counter.on_wait_done([&scheme]() {
            (..., scheme.template get<types>().unlock_writes());
        });
#endif
    }

private:
    scheme_view_from_partition();
};
