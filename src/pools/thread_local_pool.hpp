#pragma once

#include <iterator>
#include <set>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/pool/pool.hpp>



template <typename T, uint8_t max_threads>
class thread_local_pool
{
    struct pool_node
    {
        pool_node(thread_local_pool* pool);

        boost::pool<> pool;
        std::vector<T*> free_list;
        std::vector<T*> freed_while_rebalancing;
    };

    enum class status
    {
        IDLE,
        WORKING,
        REBALANCING
    };

public:
    thread_local_pool();

    template <typename... Args>
    T* get(Args&&... args);

    void release(T* object);

    void this_thread_sinks();
    void rebalance();

private:
    inline void decrease_worker_count(status expected) noexcept;
    inline void decrease_worker_count_nonforced(status expected) noexcept;
    inline pool_node& get_node() noexcept;

private:
    std::array<pool_node*, max_threads> _nodes;
    std::array<std::thread::id, max_threads> _ids;
    std::atomic<uint8_t> _index;
    uint32_t _free_max_approx;

    bool _has_sink_thread;
    std::set<uint8_t> _sink_threads;

    std::atomic<status> _status;
    std::atomic<uint8_t> _worker_count;
};


template <typename T, uint8_t max_threads>
thread_local_pool<T, max_threads>::pool_node::pool_node(thread_local_pool* pool) :
    pool(sizeof(T))
{
    auto index = pool->_index++;
    assert(pool->_nodes[index] == nullptr);
    assert(index < max_threads);

    pool->_nodes[index] = this;
    pool->_ids[index] = std::this_thread::get_id();
}

template <typename T, uint8_t max_threads>
thread_local_pool<T, max_threads>::thread_local_pool() :
    _nodes(),
    _index(0),
    _free_max_approx(0),
    _has_sink_thread(false),
    _status(status::IDLE),
    _worker_count(0)
{}

template <typename T, uint8_t max_threads>
template <typename... Args>
T* thread_local_pool<T, max_threads>::get(Args&&... args)
{
    auto& node = get_node();

    // We are working now
    ++_worker_count;

    // Case A) Pool is IDLE or WORKING (ie. another thread), do a full get
    status expected_idle = status::IDLE;
    status expected_working = status::WORKING;
    if (_status.compare_exchange_weak(expected_idle, status::WORKING) || _status.compare_exchange_weak(expected_working, status::WORKING))
    {

        void* ptr = nullptr;
        if (node.free_list.empty())
        {
            ptr = node.pool.malloc();
        }
        else
        {
            ptr = node.free_list.back();
            node.free_list.pop_back();
        }

        decrease_worker_count(status::WORKING);

        auto object = new (ptr) T(std::forward<Args>(args)...);
        return object;
    }

    decrease_worker_count_nonforced(status::WORKING);

    // Case B) Pool is currently rebalancing (might have ended by now, we do not care), simply alloc a new object
    // in any case, do not change status
    void* ptr = node.pool.malloc();
    return new (ptr) T(std::forward<Args>(args)...);
}

template <typename T, uint8_t max_threads>
void thread_local_pool<T, max_threads>::release(T* object)
{
    // We are working now
    ++_worker_count;

    // Destroy object now
    auto& node = get_node();
    std::destroy_at(object);

    status expected_idle = status::IDLE;
    status expected_working = status::WORKING;
    if (_status.compare_exchange_strong(expected_idle, status::WORKING) || _status.compare_exchange_strong(expected_working, status::WORKING))
    {
        node.free_list.push_back(object);

        for (auto other : node.freed_while_rebalancing)
        {
            node.free_list.push_back(other);
        }
        node.freed_while_rebalancing.clear();

        if (!_has_sink_thread ||
            !std::any_of(_sink_threads.begin(), _sink_threads.end(), [this](auto& val) { return _ids[val] == std::this_thread::get_id(); }))
        {
            _free_max_approx = std::max(_free_max_approx, (uint32_t)node.free_list.size());
        }

        decrease_worker_count(status::WORKING);
        return;
    }

    // Put them in a pending list
    node.freed_while_rebalancing.push_back(object);
    decrease_worker_count_nonforced(status::WORKING);
}

template <typename T, uint8_t max_threads>
void thread_local_pool<T, max_threads>::this_thread_sinks()
{
    // Make sure to trigger a node, just in case we are no already in
    auto& node = get_node();

    // Flag ourselves
    for (uint8_t i = 0, total_threads = _index; i < total_threads; ++i)
    {
        if (_ids[i] == std::this_thread::get_id())
        {
            _has_sink_thread = true;
            _sink_threads.insert(i);
            break;
        }
    }
}


// TODO(gpascualg): Find a better place for this
// TODO(gpascualg): Clang does not allow constexpr uniform_int
template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g) noexcept
{
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}


template <typename T, uint8_t max_threads>
void thread_local_pool<T, max_threads>::rebalance()
{
    // Only attempt rebalance if it is not working
    status expected_idle = status::IDLE;
    if (_status.compare_exchange_strong(expected_idle, status::REBALANCING))
    {
        if (!_has_sink_thread)
        {
            // TODO(gpascualg): Rebalance non-sink impl.
            assert(false);

            _status = status::IDLE;
            return;
        }

        // Rebalance only once there is enough candidates
        uint8_t total_threads = _index;
        if (_free_max_approx >= 256 / total_threads)
        {
            _status = status::IDLE;
            return;
        }

        _free_max_approx = 0;

        for (uint8_t i = 0; i < total_threads; ++i)
        {
            if (_sink_threads.find(i) != _sink_threads.end())
            {
                continue;
            }

            // TODO(gpascualg): Find a better place for this
            static std::random_device rd;
            static std::mt19937 gen(rd());

            auto sink_idx = select_randomly(_sink_threads.begin(), _sink_threads.end(), gen);
            std::copy(_nodes[i]->free_list.begin(), _nodes[i]->free_list.end(), std::back_inserter(_nodes[*sink_idx]->free_list));
            _nodes[i]->free_list.clear();
        }

        _status = status::IDLE;
    }
}

template <typename T, uint8_t max_threads>
inline void thread_local_pool<T, max_threads>::decrease_worker_count(status expected) noexcept
{
    // Change status back
    if (--_worker_count == 0)
    {
        bool changed = _status.compare_exchange_strong(expected, status::IDLE);

        // It might already be IDLE, if we are coming from a rebalancing state
        assert((changed || expected == status::IDLE) && "Changing during an unexpected state");
    }
}

template <typename T, uint8_t max_threads>
inline void thread_local_pool<T, max_threads>::decrease_worker_count_nonforced(status expected) noexcept
{
    // Change status back
    if (--_worker_count == 0)
    {
        _status.compare_exchange_strong(expected, status::IDLE);
    }
}

template <typename T, uint8_t max_threads>
inline typename thread_local_pool<T, max_threads>::pool_node& thread_local_pool<T, max_threads>::get_node() noexcept
{
    thread_local pool_node node(this);
    return node;
}
