# Lemon Cake

A programming language.

## Build it

    ./make

## TODO

- define `s1 s2 s4 s8 u1 u2 u4 u8` to `signed int 8 bits, ...`
- redefine object to contain:
    - obj type
    - whole obj size
    - ...unspecified data
  so that you can allocale with a single malloc, deallocate with a single free,
  and duplicate the object since you have the size.

## Reference

x86-64:
    Argument order: `rdi rsi rdx rcx r8 r9`.
    Return registers: `rax` and `rdx` (also used above)
    Preserved registers: `rbx rsp rbp r12 r13 r14 r15`.

## License

ISC
