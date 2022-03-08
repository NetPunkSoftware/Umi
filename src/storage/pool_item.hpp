#pragma once

#include "storage/ticket.hpp"


template <typename T>
class pool_item
{
public:
    using ticket_t = typename ::ticket<T>::ptr;

    pool_item() noexcept;
    pool_item(const pool_item&) = delete;
    pool_item(pool_item&& other) noexcept;
    pool_item& operator=(const pool_item&) = delete;
    pool_item& operator=(pool_item&& other) noexcept;

    inline bool has_ticket() const noexcept;
    inline const typename ticket<T>::ptr& ticket() const noexcept;

    inline void refresh_ticket() noexcept;

protected:
    inline void recreate_ticket() noexcept;
    inline void invalidate_ticket() noexcept;
    void invalidate() noexcept;

private:
#ifndef UMI_DEFAULT_TICKETS_TO_NULLPTR
    static inline auto _default_ticket = typename ::ticket<T>::ptr(new ::ticket<T>(nullptr));
#endif

   typename ::ticket<T>::ptr _ticket;
};

template <typename T>
pool_item<T>::pool_item() noexcept :
#ifndef UMI_DEFAULT_TICKETS_TO_NULLPTR
    _ticket(_default_ticket)
#else 
    _ticket(nullptr)
#endif
{}

template <typename T>
pool_item<T>::pool_item(pool_item&& other) noexcept :
    _ticket(std::move(other._ticket))
{
    refresh_ticket();
}

template <typename T>
pool_item<T>& pool_item<T>::operator=(pool_item&& other) noexcept
{
    _ticket = std::move(other._ticket);
    refresh_ticket();
    return *this;
}

template <typename T>
inline bool pool_item<T>::has_ticket() const noexcept
{ 
    return _ticket != nullptr;
}

template <typename T>
inline const typename ticket<T>::ptr& pool_item<T>::ticket() const noexcept
{ 
    return _ticket; 
}

template <typename T>
inline void pool_item<T>::recreate_ticket() noexcept
{
    _ticket = typename ::ticket<T>::ptr(new ::ticket<T>(reinterpret_cast<T*>(this)));
}

template <typename T>
inline void pool_item<T>::refresh_ticket() noexcept
{
    _ticket->_ptr = reinterpret_cast<T*>(this);
}

template <typename T>
inline void pool_item<T>::invalidate_ticket() noexcept
{
    _ticket->invalidate();
    _ticket = nullptr;
}


template <typename T>
void pool_item<T>::invalidate() noexcept
{
    _ticket->invalidate();
    _ticket = nullptr;
}
