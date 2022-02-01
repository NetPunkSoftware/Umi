#pragma once

#include <type_traits>

template <typename T, typename D, typename... Args>
concept has_scheme_information = std::is_base_of_v<T, D> && 
    requires(D d, Args&&... args)
    {
        { (d.*(static_cast<void(D::*)(typename std::unwrap_ref_decay_t<Args>...)>(&D::scheme_information)))(std::forward<Args>(args)...) };
    };

template <typename T, typename D, typename... Args>
struct has_scheme_information_scope
{
    inline static constexpr bool value = has_scheme_information<T, D, Args...>;
};

template <typename T, typename D, template <typename...> typename S, typename... components>
inline constexpr bool has_scheme_information_v = has_scheme_information_scope<T, D, S<components...>&>::value;