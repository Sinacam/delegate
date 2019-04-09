/*
    delegate is a lightweight std::function that only keeps a reference to the target.

    Why do we need yet another delegate?
    All other existing implementations going for the same use case all have unwieldly
    interfaces, as well as not being even remotely similar to std::function.
    delegate aims to be almost drop-in replaceable with std::function. Unless otherwise
    noted, delegate has the exact same interface as std::function.

    delegate is designed to be efficient, its only overhead in calling functors is the compiler
    not being able to inline the calls.

    delegate is cheap to copy, it should typically be passed by value.

    Calling an empty delegate is undefined behaviour, it does __not__ throw an exception.

    delegate doesn't keep the referenced target alive, use std::function for that.

    delegate doesn't support retrieving its target or the target's typeid.

    Different delegates can't be converted even if sufficiently similar, wrap in a lambda instead.

    is_delegate and is_delegate_v checks whether a type is a delegate.

    When wrapping function pointers with delegate, check if the platform supports casting a function pointer
    to void*.

    Requires C++17.
*/

#ifndef DELEGATE_HPP_INCLUDED
#define DELEGATE_HPP_INCLUDED

#include <utility>

template <typename>
class delegate;

template <typename>
struct is_delegate : std::false_type
{
};

template <typename F>
struct is_delegate<delegate<F>> : std::true_type
{
};

template <typename T>
inline constexpr bool is_delegate_v = is_delegate<T>::value;

template <typename R, typename... Args>
class delegate<R(Args...)>
{
  public:
    using result_type = R;

    delegate() noexcept : obj{}, f{} {}
    delegate(const delegate&) noexcept = default;

    template <typename T,
              std::enable_if_t<std::is_function_v<T> && std::is_invocable_r_v<R, T, Args&&...>,
                               bool> = true>
    delegate(T* t) : obj{(void*)t}, f{invoker<T>}
    {
        static_assert(sizeof(void*) == sizeof(T*),
                      "Platform doesn't support casting function pointer to void*");
    }

    template <typename T, std::enable_if_t<std::is_invocable_r_v<R, T, Args&&...> &&
                                               !is_delegate_v<std::remove_cv<T>>,
                                           bool> = true>
    delegate(T& t) : obj{&t}, f{invoker<T>}
    {
    }

    template <typename... Ts>
    auto operator()(Ts&&... ts)
        -> std::enable_if_t<std::is_invocable_r_v<R, R(Args&...), Ts&&...>, R>
    {
        if constexpr(std::is_void_v<R>)
            f(obj, ts...);
        else
            return f(obj, ts...);
    }

    operator bool() const noexcept { return f; }

    bool operator==(const delegate& rhs) const noexcept { return obj == rhs.obj && f == rhs.f; }

    bool operator!=(const delegate& rhs) const noexcept { return !((*this) == rhs); }

  private:
    void* obj;
    R (*f)(void*, Args&...);

    template <typename T>
    static R invoker(void* obj, Args&... args)
    {
        auto& t = *(T*)obj;
        if constexpr(std::is_void_v<R>)
            t(std::forward<Args>(args)...);
        else
            return t(std::forward<Args>(args)...);
    }
};

namespace detail
{
    template <typename>
    struct call;

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...)>
    {
        using type = R(Args...);
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) const> : call<R (C::*)(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) volatile> : call<R (C::*)(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) const volatile> : call<R (C::*)(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...)&> : call<R (C::*)(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) const&> : call<R (C::*)(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) volatile&> : call<R (C::*)(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) const volatile&> : call<R (C::*)(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) &&> : call<R (C::*)(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) const&&> : call<R (C::*)(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) volatile&&> : call<R (C::*)(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) const volatile&&> : call<R (C::*)(Args...)>
    {
    };

    template <typename F>
    using call_t = typename call<F>::type;
} // namespace detail

template <typename F>
delegate(F*)->delegate<F>;

template <typename T>
delegate(T&)->delegate<detail::call_t<decltype(&T::operator())>>;

#endif // DELEGATE_HPP_INCLUDED
