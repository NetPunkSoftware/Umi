#pragma once

#include <string_view>
#include <type_traits>

// If you can't use C++17's standard library, you'll need to use the GSL 
// string_view or implement your own struct (which would not be very difficult,
// since we only need a few methods here)

template <typename T> constexpr std::string_view type_name();
template <typename T> constexpr uint32_t type_hash();

template <>
constexpr std::string_view type_name<void>()
{ return "void"; }

namespace detail {

using type_name_prober = void;

template <typename T>
constexpr std::string_view wrapped_type_name() 
{
#ifdef __clang__
    return __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
    return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    return __FUNCSIG__;
#else
#error "Unsupported compiler"
#endif
}

constexpr std::size_t wrapped_type_name_prefix_length() { 
    return wrapped_type_name<type_name_prober>().find(type_name<type_name_prober>()); 
}

constexpr std::size_t wrapped_type_name_suffix_length() { 
    return wrapped_type_name<type_name_prober>().length() 
        - wrapped_type_name_prefix_length() 
        - type_name<type_name_prober>().length();
}

} // namespace detail

template <typename T>
constexpr std::string_view type_name() {
    constexpr auto wrapped_name = detail::wrapped_type_name<T>();
    constexpr auto prefix_length = detail::wrapped_type_name_prefix_length();
    constexpr auto suffix_length = detail::wrapped_type_name_suffix_length();
    constexpr auto type_name_length = wrapped_name.length() - prefix_length - suffix_length;
    return wrapped_name.substr(prefix_length, type_name_length);
}

template <typename T>
constexpr uint32_t type_hash() 
{
    constexpr auto name = type_name<T>();
    uint32_t hash = 2166136261;

    for (auto c : name)
    {
        hash = hash ^ c;
        hash = hash * 16777619;
    }
    
    return hash;
}