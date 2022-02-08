#pragma once

#include <inttypes.h>
#include <atomic>

#include <boost/intrusive_ptr.hpp>


template <typename T>
class component;


template <typename T>
class ticket
{
    template <typename D, typename E, uint16_t I, typename R> friend class pooled_static_vector;
    template <typename D> friend class pool_item;

    template <typename D> friend void intrusive_ptr_add_ref(ticket<D>* x);
    template <typename D> friend void intrusive_ptr_release(ticket<D>* x);

public:
    using ptr = boost::intrusive_ptr<ticket<T>>;

    ticket(T* ptr);

    inline auto get() const
    {
        return _ptr->derived();
    }
    
    inline auto operator->() const
    {
        return _ptr->derived();
    }

    inline bool valid() const;

private:
    inline void invalidate();

private:
    T* _ptr;
    std::atomic<uint32_t> _refs;
};

template <typename T>
using ticket_of_t = typename ::ticket<component<T>>::ptr;


template <typename T>
ticket<T>::ticket(T* ptr) :
    _ptr(ptr),
    _refs(0)
{}

template <typename T>
inline bool ticket<T>::valid() const
{
    return _ptr != nullptr;
}

template <typename T>
inline void ticket<T>::invalidate()
{ 
    _ptr = nullptr;
}

template <typename T>
inline bool operator==(ticket<T> lhs, ticket<T> rhs)
{
    return lhs.get() == rhs.get();
}

template <typename T>
inline void intrusive_ptr_add_ref(ticket<T>* x)
{
    ++x->_refs;
}

template <typename T>
inline void intrusive_ptr_release(ticket<T>* x)
{
    if(--x->_refs == 0) 
    {
        delete x;
    }
}
