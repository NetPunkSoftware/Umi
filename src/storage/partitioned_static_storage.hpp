#pragma once

#include "storage/storage.hpp"

#include <range/v3/view/slice.hpp>
#include <range/v3/view/transform.hpp>

#include <array>


template <pool_item_derived T, uint32_t N>
class partitioned_static_storage
{
    template <template <typename, uint32_t> typename storage, typename D, uint32_t M>
    friend class orchestrator;
    
public:
    static constexpr inline uint8_t tag = storage_tag(storage_grow::fixed, storage_layout::partitioned);

    using base_t = component<T>;
    using derived_t = T;
    using orchestrator_t = orchestrator<partitioned_static_storage, T, N>;

    partitioned_static_storage() noexcept;
    ~partitioned_static_storage() noexcept;

    partitioned_static_storage(partitioned_static_storage&& other) noexcept = default;
    partitioned_static_storage& operator=(partitioned_static_storage&& other) noexcept = default;

    template <typename... Args>
    T* push(bool predicate, Args&&... args) noexcept;
    T* push_ptr(bool predicate, T* object) noexcept;

    template <typename... Args>
    void pop(T* obj, Args&&... args) noexcept;

    T* change_partition(bool predicate, T* obj) noexcept;
    
    void clear() noexcept;
    
    inline auto range() noexcept
    {
        return ranges::views::transform(
            ranges::views::slice(_data, static_cast<uint16_t>(0), static_cast<std::size_t>(_current - &_data[0])),
            [](T& obj) { return &obj; });
    }
    
    inline auto range_until_partition() noexcept
    {
        return ranges::views::transform(
            ranges::views::slice(_data, 0, static_cast<std::size_t>(_partition - &_data[0])),
            [](T& obj) { return &obj; });
    }
    
    inline auto range_from_partition() noexcept
    {
        return ranges::views::transform(
            ranges::views::slice(_data, static_cast<std::size_t>(_partition - &_data[0]), static_cast<std::size_t>(_current - &_data[0])),
            [](T& obj) { return &obj; });
    }

    inline uint32_t size() const noexcept;
    inline uint32_t size_until_partition() const noexcept;
    inline uint32_t size_from_partition() const noexcept;
    inline bool empty() const noexcept;
    inline bool full() const noexcept;

    inline bool partition(T* obj) const noexcept;

private:
    void release(T* obj) noexcept;

private:
    std::array<T, N> _data;
    T* _current;
    T* _partition;
};


template <pool_item_derived T, uint32_t N>
partitioned_static_storage<T, N>::partitioned_static_storage() noexcept :
    _data(),
    _current(&_data[0]),
    _partition(&_data[0])
{}

template <pool_item_derived T, uint32_t N>
partitioned_static_storage<T, N>::~partitioned_static_storage() noexcept
{
    clear();
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
T* partitioned_static_storage<T, N>::push(bool predicate, Args&&... args) noexcept
{
    assert(_current < &_data[0] + N && "Writing out of bounds");
    T* obj = _current;
    if (predicate)
    {
        // Move partition to last
        if (_current != _partition)
        {
            *_current = std::move(*_partition);
        }

        // Increment partition and write
        obj = _partition++;
    }

    ++_current;
    static_cast<base_t&>(*obj).recreate_ticket();
    static_cast<base_t&>(*obj).base_construct(std::forward<Args>(args)...);
    return obj;
}

template <pool_item_derived T, uint32_t N>
T* partitioned_static_storage<T, N>::push_ptr(bool predicate, T* object) noexcept
{
    assert(_current < &_data[0] + N && "Writing out of bounds");
    T* obj = _current;
    if (predicate)
    {
        // Move partition to last
        if (_current != _partition)
        {
            *_current = std::move(*_partition);
        }

        // Increment partition and write
        obj = _partition++;
    }

    ++_current;
    if (obj != object)
    {
        *obj = std::move(*object);
    }

    return obj;
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
void partitioned_static_storage<T, N>::pop(T* obj, Args&&... args) noexcept
{
    static_cast<base_t&>(*obj).base_destroy(std::forward<Args>(args)...); 
    static_cast<base_t&>(*obj).invalidate_ticket();
    release(obj);
}

template <pool_item_derived T, uint32_t N>
void partitioned_static_storage<T, N>::release(T* obj) noexcept
{
    assert(obj >= &_data[0] && obj < &_data[0] + size() && "Attempting to release an object from another storage");

    if (partition(obj))
    {
        // True predicate, move partition one down and move that one
        if (auto candidate = --_partition; obj != candidate)
        {
            *obj = std::move(*candidate);
        }

        // And now fill partiton again
        if (auto candidate = --_current; _partition != candidate)
        {
            *_partition = std::move(*candidate);
        }
    }
    else if (obj != --_current)
    {
        *obj = std::move(*_current);
    }
}

template <pool_item_derived T, uint32_t N>
T* partitioned_static_storage<T, N>::change_partition(bool predicate, T* obj) noexcept
{
    assert(predicate != partition(obj) && "Can't change to the same partition");

    if (predicate)
    {
        // Moving from false (>= partition) to true (< partition)
        if (obj != _partition)
        {
            std::swap(*_partition, *obj);
        }

        // Now move partition
        obj = _partition;
        ++_partition;
    }
    else
    {
        if (auto candidate = _partition - 1; obj != candidate)
        {
            std::swap(candidate, obj);
        }

        // Move partition
        obj = _partition - 1;
        --_partition;
    }

    return obj;
}

template <pool_item_derived T, uint32_t N>
void partitioned_static_storage<T, N>::clear() noexcept
{
    for (auto obj : range())
    {
        static_cast<base_t&>(*obj).base_destroy();
        static_cast<base_t&>(*obj).invalidate_ticket();
    }

    _current = _partition = &_data[0];
}

template <pool_item_derived T, uint32_t N>
inline uint32_t partitioned_static_storage<T, N>::size() const noexcept
{
    return _current - &_data[0];
}

template <pool_item_derived T, uint32_t N>
inline uint32_t partitioned_static_storage<T, N>::size_until_partition() const noexcept
{
    return _partition - &_data[0];
}

template <pool_item_derived T, uint32_t N>
inline uint32_t partitioned_static_storage<T, N>::size_from_partition() const noexcept
{
    return _partition - _current;
}

template <pool_item_derived T, uint32_t N>
inline bool partitioned_static_storage<T, N>::empty() const noexcept
{
    return size() == 0;
}

template <pool_item_derived T, uint32_t N>
inline bool partitioned_static_storage<T, N>::full() const noexcept
{
    return _current == &_data[0] + N;
}

template <pool_item_derived T, uint32_t N>
inline bool partitioned_static_storage<T, N>::partition(T* obj) const noexcept
{
    return obj < _partition;
}
