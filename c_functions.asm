section .data

libcso_str: db "libc.so", 0

strlen_str: db "strlen", 0
malloc_str: db "malloc", 0
fprintf_str: db "fprintf", 0
free_str: db "free", 0
memcpy_str: db "memcpy", 0
getline_str: db "getline", 0
exit_str: db "exit", 0


first_function_to_load:

global c_strlen
c_strlen: dq 0
dq strlen_str

global c_malloc
c_malloc: dq 0
dq malloc_str

global c_fprintf
c_fprintf: dq 0
dq fprintf_str

global c_free
c_free: dq 0
dq free_str

global c_memcpy
c_memcpy: dq 0
dq memcpy_str

global c_getline
c_getline: dq 0
dq getline_str

global c_exit
c_exit: dq 0
dq exit_str

end_of_functions_to_load:

section .text

load_c_functions:
  push rbx
  push rbp

  ; Get a reference for "libc.so".
  mov esi, 1 ; RTLD_LAZY
  mov edi, libcso_str
  call dlopen

  ; rbx is the dlopen handle for "libc.so".
  mov rbx, rax

  ; rbp is holds the position of the function pointer and `rbp + 8` holds the
  ; position of the name of the function.
  mov rbp, first_function_to_load

load_next_function:
  mov rdi, rbx
  mov rsi, [rbp + 8]
  call dlsym
  mov [rbp], rax

  add rbp, 16
  cmp rbp, end_of_functions_to_load
  je done_loading
  jmp load_next_function

done_loading:

  pop rbp
  pop rbx

  ret
