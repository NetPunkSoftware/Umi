#pragma once

#include <type_traits>

template <typename T, typename D>
concept has_scheme_created = std::is_base_of_v<T, D> && 
    requires(D d)
    {
        { (d.*(static_cast<void(D::*)()>(&D::scheme_created)))() };
    };

template <typename T, typename D>
struct has_scheme_created_scope
{
    inline static constexpr bool value = has_scheme_created<T, D>;
};

template <typename T, typename D>
inline constexpr bool has_scheme_created_v = has_scheme_created_scope<T, D>::value;