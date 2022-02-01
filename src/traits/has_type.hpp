#pragma once

#include <tao/tuple/tuple.hpp>

#include <type_traits>


template <typename T, typename Tuple>
struct has_type;

template <typename T, typename... Us>
struct has_type<T, tao::tuple<Us...>> : std::disjunction<std::is_base_of<typename Us::derived_t, T>..., std::is_same<T, Us>...> {};
