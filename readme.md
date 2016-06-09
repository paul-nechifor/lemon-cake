# LemonCake

A programming language.

## Build it

    ./make

## Run all the tests

    all=1 ./make

## Planning

- Use `@` for quoting instead of `'` in Lisp.
- Set variables.
- Read variables.
- Use unique symbols.
- Change `new_object_t` to only malloc the size that's required for the object.
- Use the symbol code for hashing.
- Be able to run multiple lines.
- Define `s1 s2 s4 s8 u1 u2 u4 u8` to `signed int 8 bits, ...`
- Allow circular dependencies.
- Per thread GC?

## Reference

x86-64:
 * Argument order: `rdi rsi rdx rcx r8 r9`.
 * Return registers: `rax` and `rdx` (also used above)
 * Preserved registers: `rbx rsp rbp r12 r13 r14 r15`.

## License

ISC
