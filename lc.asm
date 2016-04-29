extern exit
extern c_main

section .text

global main
main:

  call c_main

  mov edi, 0
  call exit
