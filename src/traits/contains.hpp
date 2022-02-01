#pragma once

#include <tuple>
#include <type_traits>


template<typename What, typename ... Args>
struct contains {
    static constexpr bool value {(std::is_same_v<What, Args> || ...)};
};
