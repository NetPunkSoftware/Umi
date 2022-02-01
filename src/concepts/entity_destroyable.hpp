#pragma once

#include <type_traits>

template <typename T, typename D, typename... Args>
concept entity_destroyable = std::is_base_of_v<T, D> && 
    requires(D d, Args&&... args)
    {
        { (d.*(static_cast<void(D::*)(typename std::unwrap_ref_decay_t<Args>...)>(&D::entity_destroy)))(std::forward<Args>(args)...) };
    } && 
    !requires(T t, Args&&... args)
    {
        { (t.*(static_cast<void(D::*)(typename std::unwrap_ref_decay_t<Args>...)>(&D::entity_destroy)))(std::forward<Args>(args)...) };
    };

template <typename T, typename D, typename... Args>
struct entity_destroyable_scope
{
    inline static constexpr bool value = entity_destroyable<T, D, Args...>;
};

template <typename T, typename D, typename... Args>
inline constexpr bool entity_destroyable_v = entity_destroyable_scope<T, D, Args...>::value;


template <typename D>
class X
{
    friend class F;

public:
    constexpr X() = default;

private:
    void entity_destroy(float x)
    {}

    constexpr bool test()
    {
        return entity_destroyable_v<X<D>, D, float>;
    }
};

class Y_y : public X<Y_y>
{
public:
    constexpr Y_y() = default;

    void entity_destroy(float x)
    {}
};

class Y_n : public X<Y_n>
{
public:
    constexpr Y_n() = default;
};


static_assert(std::is_member_pointer_v<decltype(&Y_y::entity_destroy)>);
static_assert(entity_destroyable_v<X<Y_y>, Y_y, float>, "Can't call in correct class");
static_assert(!entity_destroyable_v<X<Y_n>, Y_n, float>, "Shouldn't be able to call");


class F
{
public:
    constexpr F() = default;

    template <typename T>
    constexpr bool test()
    {
        return T{}.test();
    }
};


static_assert(F().test<Y_y>(), "Can't call in correct class");
static_assert(!F().test<Y_n>(), "Shouldn't be able to call");