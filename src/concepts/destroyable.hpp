#pragma once

#include <type_traits>

template <typename T, typename D, typename... Args>
concept destroyable = std::is_base_of_v<T, D> && 
    requires(D d, Args&&... args)
    {
        { (d.*(static_cast<void(D::*)(typename std::unwrap_ref_decay_t<Args>...)>(&D::destroy)))(std::forward<Args>(args)...) };
    };

template <typename T, typename D, typename... Args>
struct destroyable_scope
{
    inline static constexpr bool value = destroyable<T, D, Args...>;
};

template <typename T, typename D, typename... Args>
inline constexpr bool destroyable_v = destroyable_scope<T, D, Args...>::value;