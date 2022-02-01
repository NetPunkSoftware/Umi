#pragma once

#include <cstddef>

#include <synchronization/mutex.hpp>
#include <boost/pool/pool.hpp>


template <typename T>
class plain_pool
{
public:
    plain_pool(std::size_t size);

    template <typename... Args>
    T* get(Args&&... args);

    void free(T* object);

private:
    boost::pool<> _pool;
    np::mutex _mutex;
};


template <typename T>
plain_pool<T>::plain_pool(std::size_t size) :
    _pool(size)
{}

template <typename T>
template <typename... Args>
T* plain_pool<T>::get(Args&&... args)
{
    _mutex.lock();
    void* ptr = _pool.malloc();
    _mutex.unlock();

    return new (ptr) T(std::forward<Args>(args)...);
}

template <typename T>
void plain_pool<T>::free(T* object)
{
    std::destroy_at(object);

    _mutex.lock();
    _pool.free(object);
    _mutex.unlock();
}
