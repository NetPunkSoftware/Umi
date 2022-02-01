#pragma once

#include "storage/storage.hpp"

#include <range/v3/view/concat.hpp>
#include <range/v3/view/slice.hpp>
#include <range/v3/view/transform.hpp>

#include <array>
#include <vector>


template <pool_item_derived T, uint32_t N>
class static_growable_storage
{
    template <template <typename, uint32_t> typename storage, typename D, uint32_t M>
    friend class orchestrator;
    
public:
    static constexpr inline uint8_t tag = storage_tag(storage_grow::growable, storage_layout::continuous);

    using base_t = component<T>;
    using derived_t = T;
    using orchestrator_t = orchestrator<static_growable_storage, T, N>;

    static_growable_storage() noexcept;
    ~static_growable_storage() noexcept;

    static_growable_storage(static_growable_storage&& other) noexcept = default;
    static_growable_storage& operator=(static_growable_storage&& other) noexcept = default;

    template <typename... Args>
    T* push(Args&&... args) noexcept;
    T* push_ptr(T* object) noexcept;

    template <typename... Args>
    void pop(T* obj, Args&&... args) noexcept;

    void clear() noexcept;
    
    inline auto range() noexcept
    {
        return ranges::views::transform(
            ranges::views::concat(
                ranges::views::slice(_data, static_cast<uint16_t>(0), static_cast<std::size_t>(_current - &_data[0])),
                _growable),
            [](T& obj) { return &obj; });
    }

    inline uint32_t size() const noexcept;
    inline bool empty() const noexcept;
    inline bool full() const noexcept;

private:
    void release(T* obj) noexcept;
    bool is_static(T* obj) const noexcept;
    bool is_static_full() const noexcept;

private:
    std::array<T, N> _data;
    T* _current;
    std::vector<T> _growable;
};


template <pool_item_derived T, uint32_t N>
static_growable_storage<T, N>::static_growable_storage() noexcept :
    _data(),
    _current(&_data[0]),
    _growable()
{
    _growable.reserve(N);
}

template <pool_item_derived T, uint32_t N>
static_growable_storage<T, N>::~static_growable_storage() noexcept
{
    clear();
}

template <pool_item_derived T, uint32_t N>
bool static_growable_storage<T, N>::is_static(T* obj) const noexcept
{
    return obj < &_data[0] + N;
}

template <pool_item_derived T, uint32_t N>
bool static_growable_storage<T, N>::is_static_full() const noexcept
{
    return _current == &_data[0] + N;
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
T* static_growable_storage<T, N>::push(Args&&... args) noexcept
{
    T* obj = _current;
    if (!is_static_full())
    {
        assert(_current < &_data[0] + N && "Writing out of bounds");
        ++_current;
    }
    else
    {
        obj = &_growable.emplace_back();
    }

    static_cast<base_t&>(*obj).recreate_ticket();
    static_cast<base_t&>(*obj).base_construct(std::forward<Args>(args)...);
    return obj;
}

template <pool_item_derived T, uint32_t N>
T* static_growable_storage<T, N>::push_ptr(T* object) noexcept
{
    T* obj = _current;
    if (!is_static_full())
    {
        assert(_current < &_data[0] + N && "Writing out of bounds");
        ++_current;
        *obj = std::move(*object);
    }
    else
    {
        obj = &_growable.emplace_back(std::move(*object));
    }
    
    return obj;
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
void static_growable_storage<T, N>::pop(T* obj, Args&&... args) noexcept
{
    static_cast<base_t&>(*obj).base_destroy(std::forward<Args>(args)...); 
    static_cast<base_t&>(*obj).invalidate_ticket();
    release(obj);
}

template <pool_item_derived T, uint32_t N>
void static_growable_storage<T, N>::release(T* obj) noexcept
{
    assert(((obj >= &_data[0] && obj < _current) || (obj >= &_growable[0] && obj < &_growable[0] + _growable.size())) 
        && "Attempting to release an object from another storage");

    if (is_static(obj))
    {
        if (auto candidate = --_current; obj != candidate)
        {
            *obj = std::move(*candidate);
        }
    }
    else
    {
        if (obj != &_growable.back())
        {
            *obj = std::move(_growable.back());
        }
        
        _growable.pop_back();
    }
}

template <pool_item_derived T, uint32_t N>
void static_growable_storage<T, N>::clear() noexcept
{
    for (auto obj : range())
    {
        static_cast<base_t&>(*obj).base_destroy();
        static_cast<base_t&>(*obj).invalidate_ticket();
    }
    
    _current = &_data[0];
    _growable.clear();
}

template <pool_item_derived T, uint32_t N>
inline uint32_t static_growable_storage<T, N>::size() const noexcept
{
    return _current - &_data[0] + _growable.size();
}

template <pool_item_derived T, uint32_t N>
inline bool static_growable_storage<T, N>::empty() const noexcept
{
    return size() == 0;
}

template <pool_item_derived T, uint32_t N>
inline bool static_growable_storage<T, N>::full() const noexcept
{
    return false;
}
