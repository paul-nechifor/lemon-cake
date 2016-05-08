extern eval_lines
extern load_c_functions


section .data

global c_strlen
c_strlen: dq 0

global c_malloc
c_malloc: dq 0

global c_fprintf
c_fprintf: dq 0

global c_free
c_free: dq 0

global c_memcpy
c_memcpy: dq 0

global c_getline
c_getline: dq 0

global c_exit
c_exit: dq 0


section .text

global main
main:

  call load_c_functions

  call eval_lines

  mov edi, 0
  call [c_exit]
