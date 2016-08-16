; The system externals.
extern dlopen
extern dlsym

; The LC externals (written in C).
extern eval_lines

; The include files.
%include "c_functions.asm"

section .data

global prog_argc_ptr
prog_argc_ptr: dq 0

section .text

global main
main:

  ; `rsp` is argc
  ; `rsp + 8` is argv[0]
  mov [prog_argc_ptr], rsp

  call load_c_functions

  call eval_lines

  mov edi, 0
  call [c_exit]
