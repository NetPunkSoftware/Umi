#pragma once

#include "storage/storage.hpp"

#include <range/v3/view/transform.hpp>

#include <array>
#include <vector>


template <pool_item_derived T, uint32_t N>
class growable_storage
{
    template <template <typename, uint32_t> typename storage, typename D, uint32_t M>
    friend class orchestrator;

public:
    static constexpr inline uint8_t tag = storage_tag(storage_grow::growable, storage_layout::continuous);

    using base_t = component<T>;
    using derived_t = T;
    using orchestrator_t = orchestrator<growable_storage, T, N>;
    
    growable_storage() noexcept;
    ~growable_storage() noexcept;

    growable_storage(growable_storage&& other) noexcept = default;
    growable_storage& operator=(growable_storage&& other) noexcept = default;

    template <typename... Args>
    T* push(Args&&... args) noexcept;
    T* push_ptr(T* obj) noexcept;

    template <typename... Args>
    void pop(T* obj, Args&&... args) noexcept;

    void clear() noexcept;
    
    inline auto range() noexcept
    {
        return ranges::views::transform(
            _data,
            [](T& obj) { return &obj; });
    }

    inline uint32_t size() const noexcept;
    inline bool empty() const noexcept;
    inline bool full() const noexcept;

private:
    void release(T* obj) noexcept;

private:
    std::vector<T> _data;
};


template <pool_item_derived T, uint32_t N>
growable_storage<T, N>::growable_storage() noexcept :
    _data()
{
    _data.reserve(N);
}

template <pool_item_derived T, uint32_t N>
growable_storage<T, N>::~growable_storage() noexcept
{
    clear();
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
T* growable_storage<T, N>::push(Args&&... args) noexcept
{
    T* obj = &_data.emplace_back();
    static_cast<base_t&>(*obj).recreate_ticket();
    static_cast<base_t&>(*obj).base_construct(std::forward<Args>(args)...);
    return obj;
}

template <pool_item_derived T, uint32_t N>
T* growable_storage<T, N>::push_ptr(T* obj) noexcept
{
    obj = &_data.emplace_back(std::move(*obj));
    return obj;
}

template <pool_item_derived T, uint32_t N>
template <typename... Args>
void growable_storage<T, N>::pop(T* obj, Args&&... args) noexcept
{
    static_cast<base_t&>(*obj).base_destroy(std::forward<Args>(args)...); 
    static_cast<base_t&>(*obj).invalidate_ticket();

    release(obj);
}

template <pool_item_derived T, uint32_t N>
void growable_storage<T, N>::release(T* obj) noexcept
{
    assert(obj >= &_data[0] && obj < &_data[0] + size() && "Attempting to release an object from another storage");
    assert(_data.size() > 0 && "Attempting to release from an empty vector");

    if (obj != &_data.back())
    {
        *obj = std::move(_data.back());
    }

    _data.pop_back();
    assert((_data.size() == 0 || _data.back().has_ticket()) && "Operation would leave the vector in an invalid state");
}

template <pool_item_derived T, uint32_t N>
void growable_storage<T, N>::clear() noexcept
{
    auto beg = _data.begin();
    auto end = _data.end();
    // HACK(gpascualg): For some reason i can't find, iterating the vector itself produces an infinite loop in MSVC
    //  Offending code:
    //      for (auto& obj : _data)
    for (auto obj : range())
    {
        static_cast<base_t&>(*obj).base_destroy();
        static_cast<base_t&>(*obj).invalidate_ticket();
    }

    _data.clear();
}

template <pool_item_derived T, uint32_t N>
inline uint32_t growable_storage<T, N>::size() const noexcept
{
    return static_cast<uint32_t>(_data.size());
}

template <pool_item_derived T, uint32_t N>
inline bool growable_storage<T, N>::empty() const noexcept
{
    return size() == 0;
}

template <pool_item_derived T, uint32_t N>
inline bool growable_storage<T, N>::full() const noexcept
{
    return false;
}
