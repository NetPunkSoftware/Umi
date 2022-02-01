#pragma once

#include <tao/tuple/tuple.hpp>

#include <tuple>
#include <type_traits>


template <class Candidate, class In>
struct base_dic;

template <class Candidate, class InCar, class... InCdr>
struct base_dic<Candidate, tao::tuple<InCar, InCdr...>>
{
  using type = typename std::conditional<
    std::is_same<Candidate, typename InCar::derived_t>::value
    , typename base_dic<InCar, tao::tuple<>>::type
    , typename base_dic<Candidate, tao::tuple<InCdr...>>::type
  >::type;
};

template <class Candidate>
struct base_dic<Candidate, tao::tuple<>>
{
  using type = Candidate;
};


template<typename...Ts>
using tuple_cat_t = decltype(std::tuple_cat(std::declval<Ts>()...));

template<typename T, typename...Ts>
using remove_t = tuple_cat_t<
    typename std::conditional<
    std::is_same<T, Ts>::value,
    std::tuple<>,
    std::tuple<Ts>
    >::type...
>;


template <typename Candidate, typename InCar, typename... InCdr>
inline constexpr const auto& orchestrator_type_impl()
{
    if constexpr (
        std::is_same_v<Candidate, InCar> ||                             // Provided candidate is already an storage type
        std::is_same_v<Candidate, typename InCar::orchestrator_t> ||    // We are given an orchestrator and we might have storages 
        std::is_same_v<Candidate, typename InCar::base_t> ||            // Provided type is the base component<X> type
        std::is_same_v<Candidate, typename InCar::derived_t>)           // Provided type is the derived X type
    {
        return std::declval<typename InCar::orchestrator_t>();
    }
    else if constexpr (sizeof...(InCdr) == 0)
    {
        //static_assert(false, "Provided type is not a storage type");
        return std::declval<InCar>();
    }
    else
    {
        return orchestrator_type_impl<Candidate, InCdr...>();
    }
}

template <typename Candidate, typename... In>
using orchestrator_type = std::remove_const_t<std::remove_reference_t<decltype(orchestrator_type_impl<Candidate, In...>())>>;
