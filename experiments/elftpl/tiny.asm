BITS 64
org 0x08048000

ehdr: ; Elf64_Ehdr
  db 0x7F, "ELF", 2, 1, 1, 0 ; e_ident
  dq 0
  dw 2                       ; e_type
  dw 0x3E                    ; e_machine
  dd 1                       ; e_version
  dq _start                  ; e_entry
  dq phdr - ehdr             ; e_phoff
  dq 0                       ; e_shoff
  dd 0                       ; e_flags
  dw phdr - ehdr             ; e_ehsize
  dw _start - phdr           ; e_phentsize
  dw 1                       ; e_phnum
  dw 0                       ; e_shentsize
  dw 0                       ; e_shnum
  dw 0                       ; e_shstrndx

phdr: ; Elf64_Phdr
  dd 1           ; p_type
  dd 5           ; p_flags
  dq 0           ; p_offset
  dq ehdr        ; p_vaddr
  dq ehdr        ; p_paddr
  dq _end - ehdr ; p_filesz
  dq _end - ehdr ; p_memsz
  dq 0x1000      ; p_align

_start:
  mov rax, 60
  mov rdi, 42
  syscall

_end:
