#pragma once

template<std::size_t N, typename Seq> struct offset_sequence;

template<std::size_t N, std::size_t... Ints>
struct offset_sequence<N, tao::seq::index_sequence<Ints...>>
{
    using type = tao::seq::index_sequence<Ints + N...>;
};
template<std::size_t N, typename Seq>
using offset_sequence_t = typename offset_sequence<N, Seq>::type;

template<typename Seq1, typename Seq> struct cat_sequence;

template<std::size_t... Ints1, std::size_t... Ints2>
struct cat_sequence<tao::seq::index_sequence<Ints1...>,
    tao::seq::index_sequence<Ints2...>>
{
    using type = tao::seq::index_sequence<Ints1..., Ints2...>;
};
template<typename Seq1, typename Seq2>
using cat_sequence_t = typename cat_sequence<Seq1, Seq2>::type;


template<typename Tuple, std::size_t... Ints>
constexpr tao::tuple<tao::tuple_element_t<Ints, Tuple>...> select_tuple(Tuple&& tuple, tao::seq::index_sequence<Ints...>)
{
    return { tao::get<Ints>(std::forward<Tuple>(tuple))... };
}

template <class T, class Tuple>
struct index_of;

template <class T, class... Types>
struct index_of<T, tao::tuple<T, Types...>> {
    static const std::size_t value = 0;
};

template <class T, class U, class... Types>
struct index_of<T, tao::tuple<U, Types...>> {
    static const std::size_t value = 1 + index_of<T, tao::tuple<Types...>>::value;
};

template<std::size_t N, typename Tuple>
constexpr auto remove_nth(Tuple&& tuple)
{
    constexpr auto size = tao::tuple_size<Tuple>::value;
    using first = tao::seq::make_index_sequence<N>;
    using rest = offset_sequence_t<N + 1, tao::seq::make_index_sequence<size - N - 1>>;
    using indices = cat_sequence_t<first, rest>;
    return select_tuple(std::forward<Tuple>(tuple), indices{});
}
