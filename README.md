delegate is a lightweight `std::function` that can be bound to arbitrary member functions
and only keeps a reference to the bound target, similar to C# delegates.

````c++
std::vector<int> v;
delegate<void(int)> d = BIND(v, emplace_back);
d(42);    // equivalent to v.emplace_back(42);
````

# Documentation
Let `F` denote a function type, e.g. `int(double)`.


### `BIND`
Binds its arguments as an anonymous type that can be used to initialize any delegate
with a suitably similar signature. The rules of similarity follows that of std::function.

`BIND` can bind a member function

```c++
std::vector<int> v;
delegate<void(int)> d = BIND(v, push_back);
```

or a free function

```c++
int frob(double);
delegate<int(double)> d = BIND(frob);
```

or any callable

```c++
auto foo = [&os = std::cout](auto... args) { (os << ... << args); };
delegate<void(int, int)> d = BIND(foo);
```

The delegate only keeps a reference to the first argument, which must be a lvalue.
Member functions may be overloaded, free functions may not.

### `CBIND`
The same as BIND but only usable when the fisrt argument is constexpr.
The first argument need not be kept alive in this case.

````c++
delegate<int()> d = CBIND(std::array{1, 2}, size);
````

Free functions may be overloaded with `CBIND`.

### `R delegate<F>::operator(Args...)`
Calls the referenced target chosen by overload resolution with given arguments. The arguments are forwarded in the same way as `std::function`.

### `delegate<F>::operator==` `delegate<F>::operator!=`
Compares two delegates for equality. Different delegate targets is guaranteed to compare unequal. The result of the same bind is guaranteed to compare equal. It is unspecified whether two binds with the same target compares equal.

### `std::hash<delegate<F>>`
Specialization of hash that is compatible with equality. Given a hash object h and two delegates x and y:

````
    (x == y) => h(x) == h(y)
    (x != y) => h(x) != h(y) with high probability
````

# Remarks

delegate is designed to be efficient, its overhead is usually only the compiler not being able to inline the calls.

delegate is cheap to copy and trivially copyable, it should typically be passed by value.

Calling empty or invalid delegates is undefined behaviour, it does __not__ throw an exception.

delegate doesn't keep the referenced target alive, use `std::function` for that.