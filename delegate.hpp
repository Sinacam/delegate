/*
    delegate is a lightweight std::function that only keeps a reference to the target,
    and can be bound to arbitrary member functions.

        std::vector<int> v;
        delegate<void(int)> d = BIND(v, emplace_back);
        d(42);    // equivalent to v.emplace_back(42);

    Documentation:

    BIND
        Binds its argument as an anonymous type that can be used to initialize any delegate
        with a suitably similar signature. The rules of similarity follows that of std::function.

        BIND can bind a member function

            delegate<void(int)> d = BIND(int_vector, push_back);

        or a free function

            delegate<void*(size_t)> d = BIND(std::malloc);

        The free function may not be overloaded, prefer CBIND instead.

    CBIND
        The same as BIND but only usable when its arguments are constexpr. The target object need
        not be kept alive in this case.

            delegate<int()> d = CBIND(std::array{1, 2}, size);

        Free functions may be overloaded with CBIND.

    R operator(Args...)
        Calls the referenced target chosen by overload resolution with given arguments.
        The arguments are forwarded in the same way as std::fucntion.

    operator==
    operator!=
        Compares two delegates for equality. Different delegate targets is guaranteed to compare unequal.
        The result of the same bind is guaranteed to compare equal. It is unspecified whether two binds
        with the same target compares equal.

    std::hash
        Specialization of hash that is compatible with equality. Given a hash object h and two
        delegates x and y:
            (x == y) => h(x) == h(y)
            (x != y) => h(x) != h(y) with high probability

    Remarks:

    delegate is designed to be efficient, its only overhead in calling functors is the compiler
    not being able to inline the calls.

    delegate is cheap to copy and trivially copyable, it should typically be passed by value.

    Calling an empty delegate is undefined behaviour, it does __not__ throw an exception.

    delegate doesn't keep the referenced target alive, use std::function for that.
*/

#ifndef DELEGATE_HPP_INCLUDED
#define DELEGATE_HPP_INCLUDED

#include <utility>
#include<cstdint>

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

namespace delegate_detail
{
    template <typename T>
    struct tag
    {
        using type = T;
    };
} // namespace delegate_detail

template <typename R, typename... Args>
class delegate<R(Args...)>
{
  public:
    using result_type = R;

    delegate() noexcept : obj{}, f{} {}

    delegate(delegate_detail::tag<void>, void* obj, R (*f)(void*, Args&...)) : obj{obj}, f{f} {}

    R operator()(Args... args)
    {
        if constexpr(std::is_void_v<R>)
            f(obj, args...);
        else
            return f(obj, args...);
    }

    explicit operator bool() const noexcept { return f; }

    friend bool operator==(delegate x, delegate y) { return x.obj == y.obj && x.f == y.f; }
    friend bool operator!=(delegate x, delegate y) { return !(x == y); }

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

    friend class std::hash<delegate>;
};

namespace std
{
    template<typename F>
    struct hash<delegate<F>>
    {
        // Stolen from boost::hash_combine
        size_t operator()(delegate<F> d)
        {
            auto k = (uintptr_t)d.obj;
            auto h = (uintptr_t)d.f;

            auto m = 0xc6a4a7935bd1e995;
            int r = 47;

            k *= m;
            k ^= k >> r;
            k *= m;

            h ^= k;
            h *= m;

            // Completely arbitrary number, to prevent 0's
            // from hashing to 0.
            h += 0xe6546b64;

            return h;
        }
    };
}

namespace delegate_detail
{
    template <typename...>
    struct typelist
    {
    };

    template <typename>
    struct call;

    template <typename R, typename... Args>
    struct call<R(Args...)>
    {
        using type = R(Args...);
        using return_type = R;
        using arg_typelist = typelist<Args...>;
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) const> : call<R(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) volatile> : call<R(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) const volatile> : call<R(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...)&> : call<R(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) const&> : call<R(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) volatile&> : call<R(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) const volatile&> : call<R(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) &&> : call<R(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) const&&> : call<R(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) volatile&&> : call<R(Args...)>
    {
    };

    template <typename C, typename R, typename... Args>
    struct call<R (C::*)(Args...) const volatile&&> : call<R(Args...)>
    {
    };

    template <typename F>
    struct call<delegate<F>> : call<F>
    {
    };

    template <typename F>
    using call_t = typename call<F>::type;

    template <typename F>
    using call_r_t = typename call<F>::return_type;

    template <typename F>
    using call_al_t = typename call<F>::arg_typelist;

    template <typename Lambda>
    struct init
    {
        Lambda l;
        template <typename F>
        operator delegate<F>()
        {
            return helper<F>(call_al_t<F>{});
        }

        template <typename F, typename... Args>
        delegate<F> helper(typelist<Args...>)
        {
            return l(tag<delegate<F>>{}, tag<Args>{}...);
        }
    };

    template <typename Lambda>
    init(Lambda &&)->init<Lambda>;

} // namespace delegate_detail

#define BIND1(obj)                                                                                 \
    ::delegate_detail::init                                                                        \
    {                                                                                              \
        [objp = &obj](auto delegate_tag, auto... arg_tags) {                                       \
            using Delegate = typename decltype(delegate_tag)::type;                                \
            using R = ::delegate_detail::call_r_t<Delegate>;                                       \
            return Delegate{                                                                       \
                ::delegate_detail::tag<void>{}, objp, [](void* p, auto&... args) -> R {            \
                    auto& o = *(decltype(objp))p;                                                  \
                    if constexpr(::std::is_void_v<R>)                                              \
                        o(::std::forward<typename decltype(arg_tags)::type>(args)...);             \
                    else                                                                           \
                        return o(::std::forward<typename decltype(arg_tags)::type>(args)...);      \
                }};                                                                                \
        }                                                                                          \
    }

#define BIND2(obj, memfn)                                                                          \
    ::delegate_detail::init                                                                        \
    {                                                                                              \
        [objp = &obj](auto delegate_tag, auto... arg_tags) {                                       \
            using Delegate = typename decltype(delegate_tag)::type;                                \
            using R = ::delegate_detail::call_r_t<Delegate>;                                       \
            return Delegate{                                                                       \
                ::delegate_detail::tag<void>{}, objp, [](void* p, auto&... args) -> R {            \
                    auto& o = *(decltype(objp))p;                                                  \
                    if constexpr(::std::is_void_v<R>)                                              \
                        o.memfn(::std::forward<typename decltype(arg_tags)::type>(args)...);       \
                    else                                                                           \
                        return o.memfn(                                                            \
                            ::std::forward<typename decltype(arg_tags)::type>(args)...);           \
                }};                                                                                \
        }                                                                                          \
    }

#define CBIND1(obj)                                                                                \
    ::delegate_detail::init                                                                        \
    {                                                                                              \
        [](auto delegate_tag, auto... arg_tags) {                                                  \
            using Delegate = typename decltype(delegate_tag)::type;                                \
            using R = ::delegate_detail::call_r_t<Delegate>;                                       \
            return Delegate{                                                                       \
                ::delegate_detail::tag<void>{}, nullptr, [](void* p, auto&... args) -> R {         \
                    if constexpr(::std::is_void_v<R>)                                              \
                        obj(::std::forward<typename decltype(arg_tags)::type>(args)...);           \
                    else                                                                           \
                        return obj(::std::forward<typename decltype(arg_tags)::type>(args)...);    \
                }};                                                                                \
        }                                                                                          \
    }

#define CBIND2(obj, memfn)                                                                         \
    ::delegate_detail::init                                                                        \
    {                                                                                              \
        [](auto delegate_tag, auto... arg_tags) {                                                  \
            using Delegate = typename decltype(delegate_tag)::type;                                \
            using R = ::delegate_detail::call_r_t<Delegate>;                                       \
            return Delegate{                                                                       \
                ::delegate_detail::tag<void>{}, nullptr, [](void* p, auto&... args) -> R {         \
                    if constexpr(::std::is_void_v<R>)                                              \
                        obj.memfn(::std::forward<typename decltype(arg_tags)::type>(args)...);     \
                    else                                                                           \
                        return obj.memfn(                                                          \
                            ::std::forward<typename decltype(arg_tags)::type>(args)...);           \
                }};                                                                                \
        }                                                                                          \
    }

#define DELEGATE_SELECT(_1, _2, x, ...) x
#define BIND(...) DELEGATE_SELECT(__VA_ARGS__, BIND2, BIND1, _)(__VA_ARGS__)
#define CBIND(...) DELEGATE_SELECT(__VA_ARGS__, CBIND2, CBIND1, _)(__VA_ARGS__)

#endif // DELEGATE_HPP_INCLUDED
