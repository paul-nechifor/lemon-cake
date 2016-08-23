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

- Add macros.
- Introduce lambdas.

- Write tests for broken programs (e.g.: `(+ 1`) and make sure they don't end up
  in an infinite loop.

- Add a default script that's run. This way large parts could be implemented in
  LemonCake instead of C/Assembly.

- Consider interning some commonly used symbols.

- Make dicts resizable.

- Eliminate cycles when using `print` by keeping track of objects that have
  already been printed.

- Define `s1 s2 s4 s8 u1 u2 u4 u8` to `signed int 8 bits, ...`
- Use `@` for quoting instead of `'` in Lisp.

- Rewrite the whole thing in Assembly (so it can self assemble in the future).

- Write an assembler in LemonCake that can assemble the LemonCake source code.

- Write a (very) basic Markdown transformer so I can Write the source code in
  literate LemonCake.

- Allow circular dependencies.
- Change `new_object_t` to only malloc the size that's required for the object.
- Per thread interpreter and GC? Communiction via messages.

# Principles

- There should be one default way of doing it, but many alternatives.
- The last expression in a file is what's exported by default. So a file
  defining the value of pi would just contain '3.14...\n'

- Very small full-featured packages.
  - full-featured means it contains code, tests, benchmarks, compilation
    messages, warning &c.

- EVERYTHING is versioned. `if` isn't a hardcoded builtin. It's whats defined in
  `lc.lang.if`.

- nested requirements. packages can contain nested versioned packages.

- should have good defaults for repository (i.e. where src, tests and the rest
  are supposed to be).

- embrace black and white color scheme since there's no way of finding out what
  something is with regexes (since it's highly context sensitive).

- everything that imports from `node.something` is equivalent to nodejs's
  require.

- top-level instructions must be asignments

- You should be able to add listeners for everything even for example accessing
  a variable. That way you can add a dynamic deprication warning even for
  variables.

- A package imported as 'lc.math.sin' can be found in './lc/math/sin', or
  './lc-math/sin', or './lc-math-sin', &c. This way you can override the global
  `lc.math.sin` by having a file `lc-math-sin`

- It should be easy to wrap everything in an expression for instrumentation
  purpuses and it could we used for dead code elimination.

- Add runtime checks (which could be skipped in live).

## Ideas (this needs to be organized better)

Code sketches:

```
a = ~
a = x ~
a = x ~ x + 1
a = ~ 4

x = if a then b else c
x =
  if a then b else c
x =
  if
    a
    b
    c
x =
  if a
    b
    c
x = if a
  b
  c

f1 = (a b) ~ a + b

f1 4 (lazy 5 + 5)

    # `lazy` is a macro that evaluates its arguments on access (in the expected
    # environment)

pi = import math@'1.0.0'

# Arguments should come in an ordered named structure. So:

(= s (ordstuct (
  (name)
  () # Everything after this point must be passed by name.
  (age)
  (:= nickname 'John')
)))

# A function that takes to string arguments and returns their concatenation.
connectWords = (firstWord str) (secondWord str) ~
  firstWord + secondWord

# Defining a type with typed attributes.
User = deftype
  name String
  email Email
  age int
  admin bool
```

## Links

What I've used for inspiratior or plan to use in the future:

* [A Whirlwind Tutorial on Creating Really Teensy ELF Executables for
  Linux][whirlwind-tutorial]

* [(How to Write a (Lisp) Interpreter (in Python))][lisp-in-python]

* [Minilisp][minilisp]

* [Baby's First Garbage Collector][baby-gc] — A very basic GC in C. This is
  similar to what LemonCake now uses.

* [CPython internals: A ten-hour codewalk through the Python interpreter source
  code][cpython-internals]

* [Verge][verve] ([repo][verve-repo])

* [Indentation-sensitive syntax for Lisp][lisp-indent]

## Reference

x86-64:
 * Argument order: `rdi rsi rdx rcx r8 r9`.
 * Return registers: `rax` and `rdx` (also used above)
 * Preserved registers: `rbx rsp rbp r12 r13 r14 r15`.

## License

ISC

[whirlwind-tutorial]: http://www.muppetlabs.com/~breadbox/software/tiny/teensy.html
[lisp-in-python]: http://norvig.com/lispy.html
[minilisp]: https://github.com/rui314/minilisp
[baby-gc]: http://journal.stuffwithstuff.com/2013/12/08/babys-first-garbage-collector/
[cpython-internals]: http://pgbovine.net/cpython-internals.htm
[verve]: http://tadeuzagallo.com/blog/introducing-verve/
[verve-repo]: https://github.com/tadeuzagallo/verve-lang
[lisp-indent]: http://srfi.schemers.org/srfi-49/srfi-49.html
