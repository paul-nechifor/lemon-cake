# LemonCake

A programming language that combines the speed of Ruby, with the flexibility of
Assembly and the consistency of Lisp.

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

- I've used `>` for code instead of indentation or triple tick. Use triple tick.

- Add function composition and refactor `doc-code`.

- Disassemble `lc` and see how many instructions are encodable by `lc`.

- Try to see if you can come up with an elf template. It would look like a list
  of bytes. What you would need to do is just add some bytes at position x and
  fix some fields based on the length of x. This will answer the question of how
  to handle sections.

- Add an argument to `new\_object` to not run gc so that I don't have to set
  `gc_is_on` all the time (setting that doesn't check it was on in the first
  place).

- How to create new objects:
  - a function that initializes the object structure.
    - a function that adds an object to the call stack (uses the function above)
  - new functions that take object structures and give their types the required
    values.


- This is how assembly should look like:

    (asm
      '
        mov rax, simbol1
      '
      (dict simbol1 12345)
    )

- It should assemble to machine code in memory that's executable.

- It should return a list with two values:

    - a number representing a pointer to the first byte of the executable code
      (it should be possible to free this with a single free call)
    - a dictionary of strings to pointers representing the exported things
      (`asm` needs to add the program offset since `assemble-bytes` returns
      origin-relative values)

- The function above is a high level function intended to be able to execute
  code dynamically. For writing to disk we need other lower level functions.

- This function generates a list of numbers representing each byte of the
  machine code with the offsets comming separatelly.

    (assemble-bytes
      'mov rax, simbol1'
      (dict simbol1 12345)
    )

- It will return:

    - a list of numbers of bytes of the machine code. all the bytes representing
      position dependent numbers will be 0.
    - a list of pairs representing numbers to be overriden where the elements are:
        - the offset where to write to
        - the number of bytes
        - the value. this number needs the program origin added to it before
          it's written.
    - a dict of exported values (relative to the origin)

- A function that writes the list of bytes:

    (write-asm-bytes
      :(0x12 0x5A 0xA9) # The list of bytes.
      0x12346754 # Pointer where to write to. Must be long enough to be able to write all of the bytes above.
      0x0020000 # The program origin. This will be the number that's added the the list of offsets.
    )

    - `asm` will use same buffer and program origin and the buffer will be in executable memory.
    - `fs-asm` will use different numbers and the write the buffer to disk

- THIS IS BROKEN? HOW WOULD YOU HANDLE `dd _end - ehdr`. I have to compute all
  the labels before generating instructions

- WHAT ABOUT sections!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

- How to assemble. (Won't support all nasm concepts.)
    - Build the environment. it will contain:
        - symbols (the dict of passed-in symbols)
        - relative-symbols (this will be populated by instructions. For example
          `_start:` will add `'_start'` with the current offset to the dict)
          (This should error if the name exists in `symbols`.)
        - relative-patches (an instruction like `jmp load_instr` cannot write
          the address directly if `load_instr` is a label and it's after the
          `jmp`). A list of:
            - a string reffering to the value that should be in
              `relative-symbols`
            - offset
            - the number of bytes (sizeof number)
        - instr (the list off instructions)
        - end (the tail of `instr`, used for appending)
        - length (the number of bytes in `instr`)
        - globals (dict of names to values. an instruction like `global _start`
          will add `_start` with `()`. An instruction like `_start:` will lookup
          to see if it needs to be exported and if so it will replace `()` with
          the actual position dependant value.)
    - Split lines by '\n' and process each line.
    - Trim whitespace. Remove comments. Ignore whitespace.
    - write the relative-patches



- The purpose of the code above is to


    this function
        (assemble-bytes 'mov rax, sim1', (dict sim1 1234))
    returns
        (:(0xa4 0x7b ...) :((4 2 0xfa4b) (2 4 232313) ...))

        - first: the list of bytes for the whole code
        - a list of 3-uples (0-based byte offset, n-bytes, start relative no.)

        In the byte list those byte positions should be all zeros. The numbers
        will have to be overriden over those places based on the relative
        position of the start of the code segment.


    input
        string of 
    output



- Add support for `$1, $2, ...` args.

- Make sure files are never imported twice.

- Look at a disassembly of the program to see which ar the most important
  instructions that need to be supported.

- Switch to using an actual stack for the stack values. This will be useful in
  the future when exceptions are introduced (since I can place handlers on the
  stack)

- Maybe include the required `.so` files here so that it works on all systems.

- Add another long GC test that uses lambdas.

- Fix `die` to accept arguments.

- Introduce types. Every variable should have a pointer to a dict which
  represents a type. The dict will need:

  - name: ?
  - super: the super type
  - methods: a dict of symbols to functions

- Add a debugging flag which will trigger printing all the evaulated
  expressions.

- Write tests for broken programs (e.g.: `(+ 1`) and make sure they don't end up
  in an infinite loop.

- Add a default script that's run. This way large parts could be implemented in
  LemonCake instead of C/Assembly.

- Make dicts resizable.

- Get rid of `stdio`, `stderr`, and `stdout` from the binary. Link to them.

- Define `s1 s2 s4 s8 u1 u2 u4 u8` to `signed int 8 bits, ...`

- Rewrite the whole thing in Assembly (so it can self assemble in the future).

- Write an assembler in LemonCake that can assemble the LemonCake source code.

- JIT. Woop woop.

- Write a (very) basic Markdown transformer so I can Write the source code in
  literate LemonCake.

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

- Try writing a simple OS (using the Linux kerner) that uses LemonCake as the
  basis for almost everything. For example a program like `ls` would just output
  data and a stylesheet and the shell manages the display.

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
  Linux][whirlwind-tutorial]: This is the article that made me love tiny
  executables.

* [(How to Write a (Lisp) Interpreter (in Python))][lisp-in-python]

* [Minilisp][minilisp]

* [Baby's First Garbage Collector][baby-gc] — A very basic GC in C. This is
  similar to what LemonCake now uses.

* [CPython internals: A ten-hour codewalk through the Python interpreter source
  code][cpython-internals]

* [Verge][verve] ([repo][verve-repo])

* [Indentation-sensitive syntax for Lisp][lisp-indent]

* [Let’s Build A Simple Interpreter][simple-interpreter]

* x86-64 instruction encoding:

  * [x86 Instruction Encoding Revealed][x86-encoding]

  * [OSDev: X86-64 Instruction Encoding][osdevx86enc]

  * [X86 Opcode and Instruction Reference][x86opref]

  * [Intel 64 Tome (23 MiB PDF)][i64tome]

  * [Linux Standard Base Core Specification for AMD64 4.1][elf64] [(or
    PDF)][elf64pdf]

  * [System V Application Binary Interface — AMD64 Architecture Processor
    Supplement (PDF)][sysvabi]

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
[simple-interpreter]: https://ruslanspivak.com/lsbasi-part1/
[x86-encoding]: http://www.codeproject.com/Articles/662301/x-Instruction-Encoding-Revealed-Bit-Twiddling-fo
[osdevx86enc]: http://wiki.osdev.org/X86-64_Instruction_Encoding
[x86opref]: http://ref.x86asm.net/coder64.html
[i64tome]: http://www.intel.co.uk/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-manual-325462.pdf
[elf64]: http://refspecs.linuxfoundation.org/LSB_4.1.0/LSB-Core-AMD64/LSB-Core-AMD64/book1.html
[elf64pdf]: https://refspecs.linuxfoundation.org/LSB_4.1.0/LSB-Core-AMD64/LSB-Core-AMD64.pdf
[sysvabi]: http://refspecs.linux-foundation.org/elf/x86_64-abi-0.95.pdf
