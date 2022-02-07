#pragma once

#include <traits/ctti.hpp>

#include "storage/ticket.hpp"
#include "common/tao.hpp"
#include "entity/component.hpp"
#include "traits/contains.hpp"

#include <inttypes.h>


template <typename D> class component;


// TODO(gpascualg): Use something along the lines of https://github.com/serge-sans-paille/frozen but with dynamic construction size?

class components_map
{
public:
    template <typename T> using bare_t = typename std::remove_pointer_t<std::decay_t<T>>;

    components_map() = default;
    components_map(components_map&& other) noexcept = default;
    components_map& operator=(components_map&& other) noexcept = default;

    template <typename... Ts>
    components_map(const tao::tuple<Ts...>& components)
    {
        // MSVC has a hard time doing everything with std::apply inside here
        emplace_components(tao::get<Ts>(components)...);
    }

    template <typename... Comps>
    inline void emplace_components(Comps&&... comps) noexcept
    {
        (..., _components.emplace(
            type_hash<bare_t<decltype(comps)>>(),
                [ticket = comps->ticket()]() {
                return reinterpret_cast<void*>(ticket->get());
            })
        );
    }

    template <typename T>
    inline T* get() const
    {
        if (auto it = _components.find(type_hash<bare_t<T>>()); it != _components.end())
        {
            return reinterpret_cast<T*>(it->second());
        }

        return nullptr;
    }

    template <typename T>
    inline void push(component<T>* component)
    {
        _components.emplace(type_hash<bare_t<T>>(), [ticket = component->ticket()]() {
            return ticket->get()->derived();
        });
    }

protected:
    explicit components_map(const components_map& other) = default;

private:
    std::unordered_map<uint32_t, std::function<void*()>> _components;
};
