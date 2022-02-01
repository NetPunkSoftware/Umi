#pragma once

#include "pools/plain_pool.hpp"


template <typename T>
class singleton_pool : public plain_pool<T>
{
public:
    static inline singleton_pool* instance = nullptr;
    static inline void make(std::size_t size);

private:
    using plain_pool<T>::plain_pool;
};



template <typename T>
inline void singleton_pool<T>::make(std::size_t size)
{
    instance = new singleton_pool<T>(size);
}

