#include <catch2/catch_all.hpp>
#include <random>

#include <entity/component.hpp>
#include <entity/scheme.hpp>
#include <storage/growable_storage.hpp>
#include <storage/partitioned_growable_storage.hpp>
#include <storage/partitioned_static_storage.hpp>
#include <storage/static_growable_storage.hpp>
#include <storage/static_storage.hpp>


class client : public component<client>
{
public:
    using component<client>::component;

    inline void construct(bool partition)
    {
        _partition = partition;
    }

    inline bool partition() const
    {
        return _partition;
    }

private:
    bool _partition;
};

constexpr uint32_t initial_size = 100;
constexpr uint32_t alloc_initial = initial_size * 2;
constexpr uint32_t random_splits = 10;

template <typename S>
inline int push_simple(S& storage, uint64_t id = 0)
{
    if constexpr (has_storage_tag(S::tag, storage_grow::none, storage_layout::partitioned))
    {
        storage.push(true, id, true);
        storage.push(false, id + 1, false);
        return 2;
    }
    else
    {
        storage.push(id);
        return 1;
    }
}

template <typename S>
inline void push_random_partition_if_available(S& storage, uint64_t id)
{
    if constexpr (has_storage_tag(S::tag, storage_grow::none, storage_layout::partitioned))
    {
        static std::random_device rd;  //Will be used to obtain a seed for the random number engine
        static std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        static std::uniform_int_distribution<> distrib(0, 1);

        bool part = (bool)distrib(gen);
        storage.push(part, id, part);
    }
    else
    {
        storage.push(id);
    }
}


template <typename O>
inline void generate_stability_tests(O& orchestrator)
{
    THEN("It can be iterated and all items are found / " + std::string(typeid(orchestrator).name()))
    {
        int count = 0;
        for (auto x : orchestrator.range())
        {
            ++count;
        }
        REQUIRE(count == orchestrator.size());
    }

    if constexpr (has_storage_tag(O::tag, storage_grow::none, storage_layout::partitioned))
    {
        THEN("Both partitions summed contain the total amount of elements / " + std::string(typeid(orchestrator).name()))
        {
            int count = 0;
            for (auto x : orchestrator.range_until_partition())
            {
                ++count;
            }

            for (auto x : orchestrator.range_from_partition())
            {
                ++count;
            }

            REQUIRE(count == orchestrator.size());
        }

        THEN("Each partition contains elements of only its own partition / " + std::string(typeid(orchestrator).name()))
        {
            for (auto x : orchestrator.range_until_partition())
            {
                REQUIRE(x->partition());
            }

            for (auto x : orchestrator.range_from_partition())
            {
                REQUIRE(!x->partition());
            }
        }
    }
}

template <template <typename, uint32_t> typename O1, template <typename, uint32_t> typename O2>
inline void generate_test_cases()
{
    orchestrator<O1, client, alloc_initial> orchestrator1;
    orchestrator<O2, client, alloc_initial> orchestrator2;

    // Populate both orchestrators
    for (int i = 0; i < initial_size; ++i)
    {
        push_random_partition_if_available(orchestrator1, i);
        push_random_partition_if_available(orchestrator2, initial_size + i);
    }

    GIVEN("Orchestrators " + std::string(typeid(orchestrator1).name()) + " and " + std::string(typeid(orchestrator2).name()))
    {
        // Do some random moves
        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> distrib(0, initial_size * 2 - 1);

        for (int i = 0; i < 100; ++i)
        {
            // Get random id
            uint64_t id = (uint64_t)distrib(gen);

            // It must be in one of them, but not both
            REQUIRE(((bool)orchestrator1.get(id) ^ (bool)orchestrator2.get(id)) == 1);

            // Perform move
            if (auto obj = orchestrator1.get(id))
            {
                orchestrator1.move(orchestrator2, obj, obj->partition());
            }
            else
            {
                obj = orchestrator2.get(id);
                orchestrator2.move(orchestrator1, obj, obj->partition());
            }

            // Now check if everything still holds
            THEN("The size matches the remaining elements")
            {
                REQUIRE(orchestrator1.size() + orchestrator2.size() == initial_size * 2);
            }

            // Now check if everything still holds
            THEN("Elements are found in one orchestrator but not in both")
            {
                for (int i = 0; i < initial_size * 2; ++i)
                {
                    REQUIRE(((bool)orchestrator1.get(id) ^ (bool)orchestrator2.get(id)) == 1);
                }
            }

            generate_stability_tests(orchestrator1);
            generate_stability_tests(orchestrator2);
        }
    }
}

SCENARIO("Tests orchestrator moves", "[orchestrator]")
{
    generate_test_cases<growable_storage, growable_storage>();
    generate_test_cases<growable_storage, partitioned_growable_storage>();
    generate_test_cases<growable_storage, partitioned_static_storage>();
    generate_test_cases<growable_storage, static_growable_storage>();
    generate_test_cases<growable_storage, static_storage>();

    generate_test_cases<partitioned_growable_storage, growable_storage>();
    generate_test_cases<partitioned_growable_storage, partitioned_growable_storage>();
    generate_test_cases<partitioned_growable_storage, partitioned_static_storage>();
    generate_test_cases<partitioned_growable_storage, static_growable_storage>();
    generate_test_cases<partitioned_growable_storage, static_storage>();

    generate_test_cases<partitioned_static_storage, growable_storage>();
    generate_test_cases<partitioned_static_storage, partitioned_growable_storage>();
    generate_test_cases<partitioned_static_storage, partitioned_static_storage>();
    generate_test_cases<partitioned_static_storage, static_growable_storage>();
    generate_test_cases<partitioned_static_storage, static_storage>();

    generate_test_cases<static_growable_storage, growable_storage>();
    generate_test_cases<static_growable_storage, partitioned_growable_storage>();
    generate_test_cases<static_growable_storage, partitioned_static_storage>();
    generate_test_cases<static_growable_storage, static_growable_storage>();
    generate_test_cases<static_growable_storage, static_storage>();

    generate_test_cases<static_storage, growable_storage>();
    generate_test_cases<static_storage, partitioned_growable_storage>();
    generate_test_cases<static_storage, partitioned_static_storage>();
    generate_test_cases<static_storage, static_growable_storage>();
    generate_test_cases<static_storage, static_storage>();
}
