#pragma once


namespace detail
{
    template<typename>
    struct tester
    {
        using result = std::false_type;
    };
    
    template<class T, class U>
    struct tester<U T::*>
    {
        using result = std::true_type;
    };

    template<typename T>
    struct inner_type
    {
        using type = T;
    };

    template <template <typename> typename R, typename U>
    struct inner_type<R<U>>
    {
        using type = std::conditional_t<std::is_same_v<std::reference_wrapper<U>, R<U>>, U&, R<U>>;
    };
}
