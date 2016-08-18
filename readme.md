# LemonCake

A highly flexible programming language.

This language is in the very early stages of being implemented. My plans for it
are very chaotic, but hopefully it will converge towards something nice.

I just want to experiment with creating a language. I don't expect any of this
to be useful. But maybe after 10 years of working on it could become a cool toy
language.

## Build it

    ./make

## Run all the tests

    all=1 ./make

## Planning

These are some of the things I plan to do, ordered by priority and bunched into
groups.

- Join all the design documents I have into this file.

- Set variables.
- Read variables.
- Use unique symbols.
- Use the symbol code for hashing.

- Think about how variable contexts should be implemented.

- Introduce lambdas.

- Define `s1 s2 s4 s8 u1 u2 u4 u8` to `signed int 8 bits, ...`
- Use `@` for quoting instead of `'` in Lisp.

- Rewrite the whole thing in Assembly (so it can self assemble in the future).

- Write an assembler in LemonCake that can assemble the LemonCake source code.

- Write a (very) basic Markdown transformer so I can Write the source code in
  literate LemonCake.

- Allow circular dependencies.
- Change `new_object_t` to only malloc the size that's required for the object.
- Per thread GC? Communiction via messages.

## Reference

x86-64:
 * Argument order: `rdi rsi rdx rcx r8 r9`.
 * Return registers: `rax` and `rdx` (also used above)
 * Preserved registers: `rbx rsp rbp r12 r13 r14 r15`.

## License

ISC
