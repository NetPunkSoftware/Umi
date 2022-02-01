#include <catch2/catch_all.hpp>

#include <entity/component.hpp>
#include <entity/scheme.hpp>
#include <storage/growable_storage.hpp>
#include <storage/partitioned_growable_storage.hpp>
#include <storage/partitioned_static_storage.hpp>
#include <storage/static_growable_storage.hpp>
#include <storage/static_storage.hpp>
#include <view/scheme_view.hpp>


class client : public component<client>
{
public:
    using component<client>::component;

    void construct()
    {
        constructor_called = false;
    }

    void construct(int i)
    {
        constructor_called = true;
    }
    
    bool constructor_called = false;
};

class npc : public component<npc>
{
public:
    using component<npc>::component;
};

class invalid_component
{};

class non_registered_component : public component<non_registered_component>
{
public:
    using component<non_registered_component>::component;
};


template <typename T, typename O, typename S, typename... Args>
auto get_args(S& scheme, Args&&... args)
{
    if constexpr (is_partitioned_storage(O::tag))
    {
        return scheme.template args<T>(true, std::forward<Args>(args)...);
    }
    else
    {
        return scheme.template args<T>(std::forward<Args>(args)...);
    }
}

template <template <typename, uint32_t> typename S>
void test_iteration_with_single_storage()
{
    GIVEN("a " + std::string(typeid(S<client, 128>).name()) + " store and a scheme with two components")
    {
        scheme_store<
            S<client, 128>,
            S<npc, 128>
        > store;

        auto scheme = scheme_maker<client, npc>()(store);

        WHEN("two entities are created in the scheme")
        {
            for (int i = 0; i < 2; ++i)
            {
                scheme.create(i, get_args<client, S<client, 128>>(scheme), get_args<npc, S<npc, 128>>(scheme));
            }

            // Waitable lifespan must be greater than its fibers
            np::fiber_pool<> pool;

            THEN("they can be iterated continuously with a view")
            {
                pool.push([&pool, &scheme] {
                    np::counter counter;
                    auto idx = 0;
                    scheme_view::continuous(counter, &pool, scheme, [&idx](auto client, auto npc)
                        {
                            REQUIRE(std::is_same_v<decltype(client), class client*>);
                            REQUIRE(std::is_same_v<decltype(npc), class npc*>);

                            REQUIRE(client->id() == idx);
                            REQUIRE(npc->id() == idx);
                            idx += 1;
                        });

                    counter.wait();
                    REQUIRE(idx == 2);
                    pool.end();
                });

                pool.start();
                pool.join();
            }

            THEN("they can be iterated in parallel with a view")
            {
                pool.push([&pool, &scheme] {
                    np::counter counter;
                    std::atomic<uint16_t> idx = 0;
                    scheme_view::parallel(counter, &pool, scheme, [&idx](auto client, auto npc)
                        {
                            REQUIRE(std::is_same_v<decltype(client), class client*>);
                            REQUIRE(std::is_same_v<decltype(npc), class npc*>);

                            // Can't do == idx as with continuous, other threads might modify it before us
                            REQUIRE(client->id() == npc->id());
                            ++idx;
                        });

                    counter.wait();
                    REQUIRE(idx == 2);
                    pool.end();
                });

                pool.start();
                pool.join();
            }
        }
    }
}
//
//template <template <typename, uint32_t> typename S1, template <typename, uint32_t> typename S2>
//void test_iteration_with_single_storage()
//{
//    GIVEN("a " + std::string(typeid(S1<client, 128>).name()) + " store, a " + std::string(typeid(S2<client, 128>).name()) + " store and a scheme with two components")
//    {
//        scheme_store<
//            S1<client, 128>,
//            S2<npc, 128>
//        > store;
//
//        if constexpr (is_partitioned(S1<client, 128>::tag))
//        {
//            //GIVEN()
//        }
//    }
//}

SCENARIO("schemes can be iterated with scheme views")
{
    test_iteration_with_single_storage<growable_storage>();
    test_iteration_with_single_storage<partitioned_growable_storage>();
    test_iteration_with_single_storage<partitioned_static_storage>();
    test_iteration_with_single_storage<static_growable_storage>();
    test_iteration_with_single_storage<static_storage>();
}

