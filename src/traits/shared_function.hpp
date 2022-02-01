#pragma once

#include <memory>


template<class F>
struct shared_function {
  std::shared_ptr<F> f;
  shared_function() = delete; // = default works, but I don't use it
  shared_function(F&& f_):f(std::make_shared<F>(std::move(f_))){}
  shared_function(shared_function const&)=default;
  shared_function(shared_function&&)=default;
  shared_function& operator=(shared_function const&)=default;
  shared_function& operator=(shared_function&&)=default;
  template<class...As>
  auto operator()(As&&...as) const {
    return (*f)(std::forward<As>(as)...);
  }
};
template<class F>
shared_function< std::decay_t<F> > make_shared_function( F&& f ) {
  return { std::forward<F>(f) };
}


template<class Sig>
struct task;

// putting it in a namespace allows us to specialize it nicely for void return value:
namespace details {
    template<class R, class...Args>
    struct task_pimpl {
        virtual R invoke(Args&&...args) const = 0;
        virtual ~task_pimpl() {};
        virtual const std::type_info& target_type() const = 0;
    };

    // store an F.  invoke(Args&&...) calls the f
    template<class F, class R, class...Args>
    struct task_pimpl_impl :task_pimpl<R, Args...> {
        F f;
        template<class Fin>
        task_pimpl_impl(Fin&& fin) :f(std::forward<Fin>(fin)) {}
        virtual R invoke(Args&&...args) const final override {
            return f(std::forward<Args>(args)...);
        }
        virtual const std::type_info& target_type() const final override {
            return typeid(F);
        }
    };

    // the void version discards the return value of f:
    template<class F, class...Args>
    struct task_pimpl_impl<F, void, Args...> :task_pimpl<void, Args...> {
        F f;
        template<class Fin>
        task_pimpl_impl(Fin&& fin) :f(std::forward<Fin>(fin)) {}
        virtual void invoke(Args&&...args) const final override {
            f(std::forward<Args>(args)...);
        }
        virtual const std::type_info& target_type() const final override {
            return typeid(F);
        }
    };
};

template<class R, class...Args>
struct task<R(Args...)> {
    // semi-regular:
    task() = default;
    task(task&&) = default;
    // no copy

private:
    // aliases to make some SFINAE code below less ugly:
    template<class F>
    using call_r = std::invoke_result_t<F const&, Args...>;
    template<class F>
    using is_task = std::is_same<std::decay_t<F>, task>;
public:
    // can be constructed from a callable F
    template<class F,
        // that can be invoked with Args... and converted-to-R:
        class = decltype((R)(std::declval<call_r<F>>())),
        // and is not this same type:
        std::enable_if_t < !is_task<F>{}, int > * = nullptr
    >
        task(F && f) :
        m_pImpl(make_pimpl(std::forward<F>(f)))
    {}

    // the meat: the call operator        
    R operator()(Args... args)const {
        return m_pImpl->invoke(std::forward<Args>(args)...);
    }
    explicit operator bool() const {
        return (bool)m_pImpl;
    }
    void swap(task& o) {
        std::swap(m_pImpl, o.m_pImpl);
    }
    template<class F>
    void assign(F&& f) {
        m_pImpl = make_pimpl(std::forward<F>(f));
    }
    // Part of the std::function interface:
    const std::type_info& target_type() const {
        if (!*this) return typeid(void);
        return m_pImpl->target_type();
    }
    template< class T >
    T* target() {
        return target_impl<T>();
    }
    template< class T >
    const T* target() const {
        return target_impl<T>();
    }
    // compare with nullptr    :    
    friend bool operator==(std::nullptr_t, task const& self) { return !self; }
    friend bool operator==(task const& self, std::nullptr_t) { return !self; }
    friend bool operator!=(std::nullptr_t, task const& self) { return !!self; }
    friend bool operator!=(task const& self, std::nullptr_t) { return !!self; }
private:
    template<class T>
    using pimpl_t = details::task_pimpl_impl<T, R, Args...>;

    template<class F>
    static auto make_pimpl(F&& f) {
        using dF = std::decay_t<F>;
        using pImpl_t = pimpl_t<dF>;
        return std::make_unique<pImpl_t>(std::forward<F>(f));
    }
    std::unique_ptr<details::task_pimpl<R, Args...>> m_pImpl;

    template< class T >
    T* target_impl() const {
        return dynamic_cast<pimpl_t<T>*>(m_pImpl.get());
    }
};
