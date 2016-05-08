; The system externals.
extern dlopen
extern dlsym

; The LC externals (written in C).
extern eval_lines

; The include files.
%include "c_functions.asm"

section .text

global main
main:

  call load_c_functions

  call eval_lines

  mov edi, 0
  call [c_exit]
