#pragma once

#include "storage/storage.hpp"

#include <range/v3/view/slice.hpp>
#include <range/v3/view/transform.hpp>

#include <array>


template <pool_item_derived T, uint32_t N>
class static_storage
{
    template <template <typename, uint32_t> typename storage, typename D, uint32_t M>
    friend class orchestrator;
    
public:
    static constexpr inline uint8_t tag = storage_tag(storage_grow::fixed, storage_layout::continuous);

    using base_t = component<T>;
    using derived_t = T;
    using orchestrator_t = orchestrator<static_storage, T, N>;

    static_storage() noexcept;
    ~static_storage() noexcept;

    static_storage(static_storage&& other) noexcept = default;
    static_storage& operator=(static_storage&& other) noexcept = default;

    template <typename... Args>
    T* push(Args&&... args) noexcept;
    T* push_ptr(T* object) noexcept;

    template <typename... Args>
    void pop(T* obj, Args&&... args) noexcept;

    void clear() noexcept;
    
    inline auto range() noexcept
    {
        return ranges::views::transform(
            ranges::views::slice(_data, static_cast<uint16_t>(0), static_cast<std::size_t>(_current - &_data[0])),
            [](T& obj) { return &obj; });
    }

    inline uint32_t size() const noexcept;
    inline bool empty() const noexcept;
    inline bool full() const noexcept;

private:
    void release(T* obj) noexcept;

private:
    std::array<T, N> _data;
    T* _current;
};


template <pool_item_derived T, uint32_t N>
static_storage<T, N>::static_storage() noexcept :
    _data(),
    _current(&_data[0])
{}


template <pool_item_derived T, uint32_t N>
static_storage<T, N>::~static_storage() noexcept
{
    clear();
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
T* static_storage<T, N>::push(Args&&... args) noexcept
{
    assert(_current < &_data[0] + N && "Writing out of bounds");
    static_cast<base_t&>(*_current).recreate_ticket();
    static_cast<base_t&>(*_current).base_construct(std::forward<Args>(args)...);
    return _current++;
}

template <pool_item_derived T, uint32_t N>
T* static_storage<T, N>::push_ptr(T* object) noexcept
{
    assert(_current < &_data[0] + N && "Writing out of bounds");
    *_current = std::move(*object);
    return _current++;
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
void static_storage<T, N>::pop(T* obj, Args&&... args) noexcept
{
    static_cast<base_t&>(*obj).base_destroy(std::forward<Args>(args)...); 
    static_cast<base_t&>(*obj).invalidate_ticket();
    release(obj);
}

template <pool_item_derived T, uint32_t N>
void static_storage<T, N>::release(T* obj) noexcept
{
    assert(obj >= &_data[0] && obj < _current && "Attempting to release an object from another storage");

    if (auto candidate = --_current; obj != candidate)
    {
        *obj = std::move(*candidate);
    }
}

template <pool_item_derived T, uint32_t N>
void static_storage<T, N>::clear() noexcept
{
    for (auto obj : range())
    {
        static_cast<base_t&>(*obj).base_destroy();
        static_cast<base_t&>(*obj).invalidate_ticket();
    }

    _current = &_data[0];
}

template <pool_item_derived T, uint32_t N>
inline uint32_t static_storage<T, N>::size() const noexcept
{
    return _current - &_data[0];
}

template <pool_item_derived T, uint32_t N>
inline bool static_storage<T, N>::empty() const noexcept
{
    return size() == 0;
}

template <pool_item_derived T, uint32_t N>
inline bool static_storage<T, N>::full() const noexcept
{
    return _current == &_data[0] + N;
}
