# delegate

delegate is a lightweight `std::function` that only keeps a reference to the target.

# Why do we need yet another delegate?
All other existing implementations going for the same use case all have unwieldly
interfaces, as well as not being even remotely similar to `std::function`. delegate aims to be almost drop-in replaceable with `std::function`. Unless otherwise noted, delegate has the exact same interface and semantics as `std::function`.

# Remarks
delegate is designed to be efficient, its only overhead in calling functors is the compiler not being able to inline the calls.

delegate is cheap to copy, it should typically be passed by value.

Calling an empty delegate is undefined behaviour, it does __not__ throw an exception.

delegate doesn't keep the referenced target alive, use `std::function` for that.

delegate doesn't support retrieving its target or the target's typeid.

Different delegates can't be converted even if sufficiently similar, wrap in a lambda instead.

`is_delegate` and `is_delegate_v` checks whether a type is a delegate.

When wrapping function pointers with delegate, check if the platform supports casting a function pointer to `void*`.

Requires C++17.