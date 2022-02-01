#pragma once

#include <type_traits>

template <typename T, typename D, typename... Args>
concept constructable = std::is_base_of_v<T, D> && 
    requires(D d, Args&&... args)
    {
        { (d.*(static_cast<void(D::*)(typename std::unwrap_ref_decay_t<Args>...)>(&D::construct)))(std::forward<Args>(args)...) };
    };

template <typename T, typename D, typename... Args>
struct constructable_scope
{
    inline static constexpr bool value = constructable<T, D, Args...>;
};

template <typename T, typename D, typename... Args>
inline constexpr bool constructable_v = constructable_scope<T, D, Args...>::value;