#pragma once

#include "common/types.hpp"
#include "storage/pool_item.hpp"
#include "entity/components_map.hpp"
#include "entity/scheme.hpp"
#include "storage/storage.hpp"

#include "concepts/constructable.hpp"
#include "concepts/destroyable.hpp"
#include "concepts/entity_destroyable.hpp"
#include "concepts/has_scheme_created.hpp"
#include "concepts/has_scheme_information.hpp"


template <typename T>
class component : public pool_item<component<T>>
{
    template <typename... vectors> friend struct scheme;
    friend class scheme_entities_map;

    // Friends with all storage types
    template <pool_item_derived D, uint32_t N> friend class growable_storage;
    template <pool_item_derived D, uint32_t N> friend class partitioned_growable_storage;
    template <pool_item_derived D, uint32_t N> friend class partitioned_static_storage;
    template <pool_item_derived D, uint32_t N> friend class static_growable_storage;
    template <pool_item_derived D, uint32_t N> friend class static_storage;

public:
    using derived_t = T;

public:
    inline entity_id_t id() const noexcept
    {
        return _id;
    }

    inline std::shared_ptr<components_map>& components() noexcept
    {
        return _components;
    }

    template <typename D>
    inline D* get() const noexcept
    {
        return _components->template get<D>();
    }

    template <typename D>
    inline void push_component(component<D>* component) noexcept
    {
        _components->template push<D>(component);
    }

    inline component<derived_t>* base()
    {
        return this;
    }

    inline derived_t* derived()
    {
        return reinterpret_cast<derived_t*> (this);
    }

private:
    template <typename... Args>
    constexpr inline void base_construct(entity_id_t id, Args&&... args);

    template <typename... Args>
    constexpr inline void base_entity_destroy(Args... args);

    template <typename... Args>
    constexpr inline void base_destroy(Args&&... args);

    constexpr inline void base_scheme_created(const std::shared_ptr<components_map>& map);

    template <template <typename...> typename S, typename... comps>
    constexpr inline void base_scheme_information(S<comps...>& scheme);

protected:
    entity_id_t _id;
    std::shared_ptr<components_map> _components;
};


template <typename derived_t>
template <typename... Args>
constexpr inline void component<derived_t>::base_construct(entity_id_t id, Args&&... args)
{
    _id = id;

#if (__DEBUG__ || FORCE_ALL_CONSTRUCTORS) && !DISABLE_DEBUG_CONSTRUCTOR
    static_cast<derived_t&>(*this).construct(std::forward<Args>(args)...);
    static_assert(constructable_v<component<derived_t>, derived_t, Args...>, "Method can be called but is not conceptually correct");
#else
    if constexpr (constructable_v<component<derived_t>, derived_t, Args...>)
    {
        static_cast<derived_t&>(*this).construct(std::forward<Args>(args)...);
    }
#endif
}

template <typename derived_t>
template <typename... Args>
constexpr inline void component<derived_t>::base_entity_destroy(Args... args)
{
    if constexpr (entity_destroyable_v<component<derived_t>, derived_t, Args...>)
    {
        static_cast<derived_t&>(*this).entity_destroy(std::forward<Args>(args)...);
    }
}

template <typename derived_t>
template <typename... Args>
constexpr inline void component<derived_t>::base_destroy(Args&&... args)
{
    if constexpr (destroyable_v<component<derived_t>, derived_t, Args...>)
    {
        static_cast<derived_t&>(*this).destroy(std::forward<Args>(args)...);
    }
}

template <typename derived_t>
constexpr inline void component<derived_t>::base_scheme_created(const std::shared_ptr<components_map>& map)
{
    _components = map;

    if constexpr (has_scheme_created_v<component<derived_t>, derived_t>)
    {
        static_cast<derived_t&>(*this).scheme_created();
    }
}

template <typename derived_t>
template <template <typename...> typename S, typename... comps>
constexpr inline void component<derived_t>::base_scheme_information(S<comps...>& scheme)
{
    if constexpr (has_scheme_information_v<component<derived_t>, derived_t, S, comps...>)
    {
        static_cast<derived_t&>(*this).scheme_information(scheme);
    }
}
