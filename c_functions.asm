%macro function_string 1
  %defstr str_value %1
  %1_str: db str_value, 0
%endmacro

%macro function_pointer_and_name 1
  global c_%1
  c_%1:
    dq 0
    dq %1_str
%endmacro

section .data

libcso_str: db "libc.so", 0

function_string exit
function_string fprintf
function_string free
function_string getline
function_string malloc
function_string memcpy
function_string memset
function_string strcmp
function_string strlen
function_string fread
function_string fclose
function_string ftell
function_string fseek
function_string fopen


first_function_to_load:

function_pointer_and_name exit
function_pointer_and_name fprintf
function_pointer_and_name free
function_pointer_and_name getline
function_pointer_and_name malloc
function_pointer_and_name memcpy
function_pointer_and_name memset
function_pointer_and_name strcmp
function_pointer_and_name strlen
function_pointer_and_name fread
function_pointer_and_name fclose
function_pointer_and_name ftell
function_pointer_and_name fseek
function_pointer_and_name fopen

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
