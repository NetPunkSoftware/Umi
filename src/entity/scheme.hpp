#pragma once

#include "common/tao.hpp"
#include "entity/components_map.hpp"
#include "storage/storage.hpp"
#include "traits/base_dic.hpp"
#include "traits/has_type.hpp"
#include "traits/tuple.hpp"
#include "traits/without_duplicates.hpp"

#include <range/v3/algorithm/partition.hpp>
#include <range/v3/view/zip.hpp>

#include <tao/tuple/tuple.hpp>
#include <spdlog/spdlog.h>


template <typename... comps>
struct scheme;

template <typename... comps>
struct scheme_store;


namespace detail
{
    template <typename O, typename component, typename... Args>
    struct scheme_arguments
    {
        using orchestrator_t = O;

        component comp;
        tao::tuple<Args...> args;
        bool predicate;
    };
}


template <typename... comps>
struct scheme_store
{
    template <typename T> using orchestrator_t = orchestrator_type<T, comps...>;

#if !defined(UMI_ENABLE_DEBUG_LOGS)
    constexpr scheme_store() noexcept = default;
    scheme_store(scheme_store&&) noexcept = default;
    scheme_store& operator=(scheme_store&&) noexcept = default;
#else
    constexpr scheme_store() noexcept :
        components()
    {
        spdlog::trace("CONSTRUCTED STORE");
    }
    scheme_store(scheme_store&& other) noexcept :
        components(std::move(other.components))
    {
        spdlog::trace("MOVED STORE");
    }
    
    scheme_store& operator=(scheme_store&& other) noexcept
    {
        spdlog::trace("OPERATOR= STORE");
        components = std::move(other.components);
        return *this;
    }
#endif

    template <typename T>
    constexpr inline auto get() noexcept -> std::add_lvalue_reference_t<orchestrator_t<T>>
    {
        return tao::get<orchestrator_t<T>>(components);
    }

    tao::tuple<orchestrator_t<comps>...> components;
};

template <typename... comps>
struct tickets_tuple : public tao::tuple<comps...>
{
    using tao::tuple<comps...>::tuple;

    template <typename T>
    inline auto valid() const noexcept
    {
        return tao::get<typename ticket<component<T>>::ptr>(*this)->valid();
    }

    template <typename T>
    inline auto get() const noexcept
    {
        return tao::get<typename ticket<component<T>>::ptr>(*this)->get()->derived();
    }
};

template <typename... Types>
tickets_tuple<std::unwrap_ref_decay_t<Types>...> make_tickets_tuple(Types&&... args)
{
    return tickets_tuple<std::unwrap_ref_decay_t<Types>...>(std::forward<Types>(args)...);
}

template <typename... comps>
struct entity_tuple : public tao::tuple<comps...>
{
    using tao::tuple<comps...>::tuple;

    inline auto tickets() noexcept
    {
        return tao::apply([](auto... args) {
            return make_tickets_tuple(args->ticket()...);
        }, downcast());
    }

    inline const tao::tuple<comps...>& downcast() const noexcept
    {
        return static_cast<const tao::tuple<comps...>&>(*this);
    }

    inline tao::tuple<comps...>& downcast() noexcept
    {
        return static_cast<tao::tuple<comps...>&>(*this);
    }

    inline constexpr uint64_t id() const noexcept
    {
        return tao::get<0>(downcast())->id();
    }

    template <typename T>
    inline constexpr auto get() noexcept
    {
        return tao::get<T*>(downcast());
    }
};

template <typename... Types>
entity_tuple<std::unwrap_ref_decay_t<Types>...> make_entity_tuple(Types&&... args)
{
    return entity_tuple<std::unwrap_ref_decay_t<Types>...>(std::forward<Types>(args)...);
}

template <typename... comps>
class scheme
{
    template <typename... T> friend class scheme;

public:
    tao::tuple<std::add_pointer_t<comps>...> components;

    template <typename T>
    using orchestrator_t = orchestrator_type<T, comps...>;
    using tuple_t = tao::tuple<std::add_pointer_t<typename comps::derived_t>...>;
    using entity_tuple_t = entity_tuple<std::add_pointer_t<typename comps::derived_t>...>;

    template <typename... T>
    constexpr scheme(scheme_store<T...>& store) noexcept :
        components(&store.template get<comps>()...)
    {
#if defined(UMI_ENABLE_DEBUG_LOGS)
        spdlog::trace("SCHEME CONSTRUCTOR");
#endif
    }

    template <typename... T>
    constexpr void reset_store(scheme_store<T...>& store) noexcept
    {
#if defined(UMI_ENABLE_DEBUG_LOGS)
        spdlog::trace("SCHEME RESET STORE");
#endif
        components = tao::tuple<std::add_pointer_t<comps>...>(&store.template get<comps>()...);
    }

    // Disallow everything, uppon move it must be manually called
    scheme(scheme&& other) noexcept = delete;
    scheme& operator=(scheme&& rhs) noexcept = delete;

    constexpr inline void clear() noexcept
    {
        (get<comps>().clear(), ...);
    }

    template <typename T>
    constexpr inline std::add_lvalue_reference_t<orchestrator_t<T>> get() const noexcept
    {
        return *tao::get<std::add_pointer_t<orchestrator_t<T>>>(components);
    }

    template <typename T>
    constexpr inline T* get(uint64_t id) const noexcept
    {
        return get<T>().get(id)->derived();
    }

    constexpr inline auto search(uint64_t id) const noexcept -> entity_tuple_t
    {
        return entity_tuple_t(get<comps>().get(id)...);
    }

    template <typename T>
    constexpr inline bool has() const noexcept
    {
        return has_type<T, tao::tuple<comps...>>::value;
    }

    template <typename T>
    constexpr inline void require() const noexcept
    {
        static_assert(has_type<T, tao::tuple<comps...>>::value, "Requirement not met");
    }

    template <typename T, typename... Args, typename = std::enable_if_t<!is_partitioned_storage(orchestrator_t<T>::tag)>>
    constexpr auto args(Args&&... args) noexcept -> detail::scheme_arguments<orchestrator_t<T>, std::add_pointer_t<orchestrator_t<T>>, std::decay_t<Args>...>
    {
        using D = orchestrator_t<T>;
        require<D>();

        return detail::scheme_arguments<orchestrator_t<T>, std::add_pointer_t<D>, std::decay_t<Args>...> {
            .comp = tao::get<std::add_pointer_t<D>>(components),
            .args = tao::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...)
        };
    }

    template <typename T, typename... Args, typename = std::enable_if_t<is_partitioned_storage(orchestrator_t<T>::tag)>>
    constexpr auto args(bool predicate, Args&&... args) noexcept -> detail::scheme_arguments<orchestrator_t<T>, std::add_pointer_t<orchestrator_t<T>>, std::decay_t<Args>...>
    {
        using D = orchestrator_t<T>;
        require<D>();

        return detail::scheme_arguments<orchestrator_t<T>, std::add_pointer_t<D>, std::decay_t<Args>...> {
            .comp = tao::get<std::add_pointer_t<D>>(components),
            .args = tao::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...),
            .predicate = predicate
        };
    }

    template <typename T, typename... Args>
    T* alloc(uint64_t id, detail::scheme_arguments<orchestrator_t<T>, std::add_pointer_t<orchestrator_t<T>>, Args...>&& args)
    {
        return create_impl(id, std::move(args));
    }

    template <typename... A>
    auto create(uint64_t id, A&&... scheme_args) noexcept
        requires (... && !std::is_lvalue_reference<A>::value)
    {
        static_assert(sizeof...(comps) == sizeof...(scheme_args), "Incomplete scheme allocation");

        // Create tuple
        auto entities = make_entity_tuple(create_impl(id, std::move(scheme_args)) ...);

        // Create dynamic content
        auto map = std::make_shared<components_map>(entities.downcast());

        // Notify of complete scheme creation
        tao::apply([&map](auto&&... entities) mutable {
            (..., entities->base()->base_scheme_created(map));
        }, entities.downcast());

        return entities;
    }

    template <typename T>
    constexpr void destroy(T* object)
    {
        if constexpr (sizeof...(comps) == 1)
        {
            destroy_impl(object);
        }
        else
        {
            using remaining_t = remove_t<T, typename comps::derived_t...>;
            destroy_proxy<T, remaining_t>(object, std::make_index_sequence<std::tuple_size_v<remaining_t>> {});
        }
    }

    template <typename... Args>
    constexpr void destroy(Args... args)
    {
        destroy_impl(std::forward<Args>(args)...);
    }
    
    constexpr void destroy(entity_tuple_t entity)
    {
        tao::apply([this](auto... args) {
            destroy_impl(args...);
        }, entity.downcast());
    }

    template <typename T>
    constexpr auto move(scheme<comps...>& to, T* object) noexcept
    {
        if constexpr (sizeof...(comps) == 1)
        {
            return move_impl(to, object);
        }
        else
        {
            using remaining_t = remove_t<T, typename comps::derived_t...>;
            return move_proxy<T, remaining_t>(to, object, std::make_index_sequence<std::tuple_size_v<remaining_t>> {});
        }
    }

    template <typename... Args>
    constexpr auto move(scheme<comps...>& to, Args... args)
    {
        static_assert(sizeof...(Args) == sizeof...(comps), "Must provide the whole entity components");
        return move_impl(to, args...);
    }

    constexpr auto move(scheme<comps...>& to, entity_tuple_t entity)
    {
        return tao::apply([this, &to](auto... args) {
            return move_impl(to, args...);
        }, entity.downcast());
    }

    constexpr inline std::size_t size() const
    {
        return tao::get<0>(components)->size();
    }

    template <typename T = std::tuple_element_t<0, std::tuple<comps...>>, typename = std::enable_if_t<is_partitioned_storage(orchestrator_t<T>::tag)>>
    constexpr inline std::size_t size_until_partition() const
    {
        return tao::get<0>(components)->size_until_partition();
    }

    template <typename T = std::tuple_element_t<0, std::tuple<comps...>>, typename = std::enable_if_t<is_partitioned_storage(orchestrator_t<T>::tag)>>
    constexpr inline std::size_t size_from_partition() const
    {
        return tao::get<0>(components)->size_from_partition();
    }

    //template <typename Sorter, typename UnaryPredicate>
    //void partition(UnaryPredicate&& p)
    //{
    //    ranges::partition(ranges::views::zip(get<comps>().range_as_ref()...),
    //        [p = std::forward<UnaryPredicate>(p)](const auto&... elems) {
    //            return p(std::get<Sorter>(std::forward_as_tuple(elems...)));
    //        });
    //}

    template <typename T>
    auto change_partition(bool p, T* object)
    {
        if constexpr (sizeof...(comps) == 1)
        {
            return change_partition_impl(p, object);
        }
        else
        {
            using remaining_t = remove_t<T, typename comps::derived_t...>;
            return change_partition_proxy<T, remaining_t>(p, object, std::make_index_sequence<std::tuple_size_v<remaining_t>> {});
        }
    }

    template <typename... T, typename... D>
    constexpr auto overlap(scheme_store<T...>& store, scheme<D...>& other) noexcept
    {
        using W = without_duplicates<scheme, scheme<D..., comps...>>;
        return W{ store };
    }

private:
    template <typename T>
    auto create_impl(uint64_t id, T&& scheme_args) noexcept
    {
        // Create by invoking with arguments
        auto entity = tao::apply([&scheme_args, &id](auto&&... args) {
            if constexpr (is_partitioned_storage(T::orchestrator_t::tag))
            {
                return scheme_args.comp->push(scheme_args.predicate, id, std::forward<std::decay_t<decltype(args)>>(args)...);
            }
            else
            {
                return scheme_args.comp->push(id, std::forward<std::decay_t<decltype(args)>>(args)...);
            }
        }, scheme_args.args);

        // Notify of creation
        entity->base()->base_scheme_information(*this);

        return entity;
    }

    template <typename T, typename tuple, std::size_t... I>
    constexpr inline void destroy_proxy(T* object, std::index_sequence<I...>)
    {
        destroy_impl(object, object->template get<std::tuple_element_t<I, tuple>>()...);
    }

    template <typename T, typename tuple, std::size_t... I>
    constexpr inline auto move_proxy(scheme<comps...>& to, T* object, std::index_sequence<I...>)
    {
        auto temp_tuple = std::tuple_cat(std::make_tuple(object), std::make_tuple(object->template get<std::tuple_element_t<I, tuple>>()...));
        return move_impl(to, std::get<typename comps::derived_t*>(temp_tuple)...);
    }

    template <typename T, typename tuple, std::size_t... I>
    constexpr inline auto change_partition_proxy(bool p, T* object, std::index_sequence<I...>)
    {
        auto temp_tuple = std::tuple_cat(std::make_tuple(object), std::make_tuple(object->template get<std::tuple_element_t<I, tuple>>()...));
        return change_partition_impl(p, std::get<typename comps::derived_t*>(temp_tuple)...);
    }

    template <typename... Args>
    constexpr inline void destroy_impl(Args... args)
    {
        static_assert(sizeof...(Args) == sizeof...(comps), "Must provide the whole entity components");
        // First call each entity_destroy
        (..., args->base_entity_destroy(std::forward<Args>(args)...));

        // Now pop
        (..., get<orchestrator_t<std::remove_pointer_t<Args>>>().pop(args));
    }

    template <typename... Ts>
    inline constexpr auto move_impl(scheme<comps...>& to, Ts&&... entities) noexcept
    {
        static_assert(sizeof...(Ts) == sizeof...(comps), "Must provide the whole entity components");

        return tao::apply([this](auto... entities) mutable {
            (entities->base()->base_scheme_information(*this), ...);
            return make_entity_tuple(entities...);
        }, tao::tuple(get<comps>().move(to.get<comps>(), entities)...));
    }

    template <typename... Ts>
    inline auto change_partition_impl(bool p, Ts... objects)
    {
        static_assert(sizeof...(Ts) == sizeof...(comps), "Must provide the whole entity components");
        
        return tao::tuple(get<comps>().change_partition(p, objects)...);
    }
};


template <typename... comps>
struct scheme_maker
{
    template <typename... T>
    inline auto constexpr operator()(scheme_store<T...>& store) noexcept
    {
        if constexpr (sizeof...(comps) == 0)
        {
            using W = without_duplicates<scheme, scheme<typename scheme_store<T...>::template orchestrator_t<T>...>>;
            return W{ store };
        }
        else
        {
            using W = without_duplicates<scheme, scheme<typename scheme_store<T...>::template orchestrator_t<comps>...>>;
            return W{ store };
        }
    }
};

template <typename... T, typename A, typename B, typename... O>
constexpr inline auto overlap(scheme_store<T...>& store, A&& a, B&& b, O&&... other) noexcept
{
    if constexpr (sizeof...(other) == 0)
    {
        return a.overlap(store, b);
    }
    else
    {
        return overlap(store, a.overlap(b), std::forward<O>(other)...);
    }
}
