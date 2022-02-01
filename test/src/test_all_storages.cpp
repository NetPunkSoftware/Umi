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

template <template <typename, uint32_t> typename T>
inline void generate_test_cases()
{
    using storage_t = T<client, initial_size>;
    using orchestrator_t = orchestrator<T, client, initial_size>;

    GIVEN("The bare storage " + std::string(typeid(storage_t).name()))
    {
        storage_t storage;
        
        WHEN("Nothing is done")
        {
            THEN("The storage is empty")
            {
                REQUIRE(storage.size() == 0);
            }

            THEN("It can be iterated but no item is found")
            {
                bool empty = true;
                for (auto x : storage.range())
                {
                    empty = false;
                }
                REQUIRE(empty);
            }
        }

        WHEN("An item is allocated")
        {
            int count = push_simple(storage);

            THEN("The size increases")
            {
                REQUIRE(storage.size() == count);
            }

            THEN("It can be iterated and an item is found")
            {
                bool empty = true;
                for (auto x : storage.range())
                {
                    empty = false;
                }
                REQUIRE(!empty);
            }
        }

        WHEN("Many items are allocated without expanding the storage")
        {
            const int max_elements = initial_size - 5;
            for (int i = 0; i < max_elements; ++i)
            {
                push_random_partition_if_available(storage, i);
            }

            THEN("The size increases")
            {
                REQUIRE(storage.size() == (has_storage_tag(storage_t::tag, storage_grow::fixed, storage_layout::none) ? initial_size - 5 : max_elements));
            }

            THEN("It can be iterated and all items are found")
            {
                int count = 0;
                for (auto x : storage.range())
                {
                    ++count;
                }
                REQUIRE(count == max_elements);
            }

            if constexpr (has_storage_tag(storage_t::tag, storage_grow::none, storage_layout::partitioned))
            {
                THEN("Both partitions summed contain the total amount of elements")
                {
                    int count = 0;
                    for (auto x : storage.range_until_partition())
                    {
                        ++count;
                    }

                    for (auto x : storage.range_from_partition())
                    {
                        ++count;
                    }

                    REQUIRE(count == storage.size());
                }

                THEN("Each partition contains elements of only its own partition")
                {
                    for (auto x : storage.range_until_partition())
                    {
                        REQUIRE(x->partition());
                    }

                    for (auto x : storage.range_from_partition())
                    {
                        REQUIRE(!x->partition());
                    }
                }
            }
        }

        WHEN("Many items are allocated")
        {
            const int max_elements = has_storage_tag(storage_t::tag, storage_grow::fixed, storage_layout::none) ? initial_size : 612;
            for (int i = 0; i < max_elements; ++i)
            {
                push_random_partition_if_available(storage, i);
            }

            if constexpr (has_storage_tag(storage_t::tag, storage_grow::fixed, storage_layout::none))
            {
                REQUIRE(storage.full());
            }

            THEN("The size increases")
            {
                REQUIRE(storage.size() == (has_storage_tag(storage_t::tag, storage_grow::fixed, storage_layout::none) ? initial_size : max_elements));
            }

            THEN("It can be iterated and all items are found")
            {
                int count = 0;
                for (auto x : storage.range())
                {
                    ++count;
                }
                REQUIRE(count == max_elements);
            }

            if constexpr (has_storage_tag(storage_t::tag, storage_grow::none, storage_layout::partitioned))
            {
                THEN("Both partitions summed contain the total amount of elements")
                {
                    int count = 0;
                    for (auto x : storage.range_until_partition())
                    {
                        ++count;
                    }

                    for (auto x : storage.range_from_partition())
                    {
                        ++count;
                    }

                    REQUIRE(count == storage.size());
                }

                THEN("Each partition contains elements of only its own partition")
                {
                    for (auto x : storage.range_until_partition())
                    {
                        REQUIRE(x->partition());
                    }

                    for (auto x : storage.range_from_partition())
                    {
                        REQUIRE(!x->partition());
                    }
                }
            }
        }
    }

    GIVEN("The orchestrator " + std::string(typeid(orchestrator_t).name()))
    {
        orchestrator_t orchestrator;

        WHEN("Nothing is done")
        {
            THEN("The storage is empty")
            {
                REQUIRE(orchestrator.size() == 0);
            }

            THEN("It can be iterated but no item is found")
            {
                bool empty = true;
                for (auto x : orchestrator.range())
                {
                    empty = false;
                }
                REQUIRE(empty);
            }
        }

        WHEN("An item is allocated")
        {
            int count = push_simple(orchestrator);

            THEN("The size increases")
            {
                REQUIRE(orchestrator.size() == count);
            }

            THEN("It can be iterated and an item is found")
            {
                bool empty = true;
                for (auto x : orchestrator.range())
                {
                    empty = false;
                }
                REQUIRE(!empty);
            }
        }

        WHEN("Many items are allocated without expanding the orchestrator")
        {
            const int max_elements = initial_size - 5;
            for (int i = 0; i < max_elements; ++i)
            {
                push_random_partition_if_available(orchestrator, i);
            }

            THEN("The size increases")
            {
                REQUIRE(orchestrator.size() == max_elements);
            }

            THEN("It can be iterated and all items are found")
            {
                int count = 0;
                for (auto x : orchestrator.range())
                {
                    ++count;
                }
                REQUIRE(count == max_elements);
            }

            if constexpr (has_storage_tag(orchestrator_t::tag, storage_grow::none, storage_layout::partitioned))
            {
                THEN("Both partitions summed contain the total amount of elements")
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

                THEN("Each partition contains elements of only its own partition")
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

        WHEN("Many items are allocated")
        {
            const int max_elements = has_storage_tag(orchestrator_t::tag, storage_grow::fixed, storage_layout::none) ? initial_size : 612;
            for (int i = 0; i < max_elements; ++i)
            {
                push_random_partition_if_available(orchestrator, i);
            }

            if constexpr (has_storage_tag(orchestrator_t::tag, storage_grow::fixed, storage_layout::none))
            {
                REQUIRE(orchestrator.full());
            }

            THEN("The size increases")
            {
                REQUIRE(orchestrator.size() == max_elements);
            }

            THEN("It can be iterated and all items are found")
            {
                int count = 0;
                for (auto x : orchestrator.range())
                {
                    ++count;
                }
                REQUIRE(count == max_elements);
            }

            if constexpr (has_storage_tag(orchestrator_t::tag, storage_grow::none, storage_layout::partitioned))
            {
                THEN("Both partitions summed contain the total amount of elements")
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

                THEN("Each partition contains elements of only its own partition")
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


        for (int split = 0; split < random_splits; ++split)
        {
            WHEN("Many items are allocated and then randomly deleted (" + std::to_string(split) + "/10 times)")
            {
                orchestrator.clear();

                std::set<uint64_t> ids;
                const int max_elements = has_storage_tag(orchestrator_t::tag, storage_grow::fixed, storage_layout::none) ? initial_size : 1012;
                for (int i = 0; i < max_elements; ++i)
                {
                    push_random_partition_if_available(orchestrator, i);
                    ids.insert(i);
                }

                if constexpr (has_storage_tag(orchestrator_t::tag, storage_grow::fixed, storage_layout::none))
                {
                    REQUIRE(orchestrator.full());
                }

                const int delete_expect = max_elements / 2;
                std::set<uint64_t> deleted_ids;
                std::sample(ids.begin(), ids.end(), std::inserter(deleted_ids, deleted_ids.begin()), delete_expect, std::mt19937{ std::random_device{}() });
                for (auto id : deleted_ids)
                {
                    REQUIRE(orchestrator.get(id) != nullptr);
                    orchestrator.pop(orchestrator.get(id));
                }

                THEN("The size matches the remaining elements")
                {
                    REQUIRE(orchestrator.size() == max_elements - deleted_ids.size());
                }

                THEN("It can be iterated and all items are found")
                {
                    int count = 0;
                    for (auto x : orchestrator.range())
                    {
                        ++count;
                    }
                    REQUIRE(count == max_elements - deleted_ids.size());
                }

                THEN("Entities in the deleted sample are not found")
                {
                    for (auto id : deleted_ids)
                    {
                        REQUIRE(orchestrator.get(id) == nullptr);
                    }
                }

                THEN("Iterating yields no deleted element")
                {
                    for (auto obj : orchestrator.range())
                    {
                        REQUIRE(!deleted_ids.contains(obj->id()));
                    }
                }

                THEN("Iterated IDs are unique")
                {
                    std::set<uint64_t> unique_ids;
                    for (auto obj : orchestrator.range())
                    {
                        REQUIRE(!unique_ids.contains(obj->id()));
                        unique_ids.insert(obj->id());
                    }
                }

                if constexpr (has_storage_tag(orchestrator_t::tag, storage_grow::none, storage_layout::partitioned))
                {
                    THEN("Both partitions summed contain the total amount of elements")
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

                    THEN("Each partition contains elements of only its own partition")
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
        }
    }
}


SCENARIO("Tests all storages types", "[storage]")
{
    generate_test_cases<growable_storage>();
    generate_test_cases<partitioned_growable_storage>();
    generate_test_cases<partitioned_static_storage>();
    generate_test_cases<static_growable_storage>();
    generate_test_cases<static_storage>();
}
