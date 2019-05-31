delegate is a lightweight `std::function` that only keeps a reference to the target,
and can be bound to arbitrary member functions.

````c++
std::vector<int> v;
delegate<void(int)> d = BIND(v, emplace_back);
d(42);    // equivalent to v.emplace_back(42);
````

# Documentation

### `BIND`
Binds its argument as an anonymous type that can be used to initialize any delegate with a suitably similar signature. The rules of similarity follows that of `std::function`.

`BIND` can bind a member function

````c++
delegate<void(int)> d = BIND(int_vector, push_back);
````

or a free function

````c++
delegate<void*(size_t)> d = BIND(std::malloc);
````

The free function may not be overloaded, prefer `CBIND` instead.

### `CBIND`
The same as `BIND` but only usable when its arguments are `constexpr`. The target object need not be kept alive in this case.

````c++
delegate<int()> d = CBIND(std::array{1, 2}, size);
````

Free functions may be overloaded with `CBIND`.

### `R operator(Args...)`
Calls the referenced target chosen by overload resolution with given arguments. The arguments are forwarded in the same way as `std::function`.

### `operator==` `operator!=`
Compares two delegates for equality. Different delegate targets is guaranteed to compare unequal. The result of the same bind is guaranteed to compare equal. It is unspecified whether two binds with the same target compares equal.

### `std::hash`
Specialization of hash that is compatible with equality. Given a hash object h and two delegates x and y:

````
    (x == y) => h(x) == h(y)
    (x != y) => h(x) != h(y) with high probability
````

# Remarks

delegate is designed to be efficient, its only overhead in calling functors is the compiler not being able to inline the calls.

delegate is cheap to copy and trivially copyable, it should typically be passed by value.

Calling an empty delegate is undefined behaviour, it does __not__ throw an exception.

delegate doesn't keep the referenced target alive, use `std::function` for that.