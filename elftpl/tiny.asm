BITS 32

org 0x08048000

ehdr: ; Elf32_Ehdr
  db 0x7F, "ELF", 1, 1, 1, 0 ; e_ident
  db 0, 0, 0, 0, 0, 0, 0, 0
  dw 2                       ; e_type
  dw 3                       ; e_machine
  dd 1                       ; e_version
  dd _start                  ; e_entry
  dd phdr - ehdr             ; e_phoff
  dd 0                       ; e_shoff
  dd 0                       ; e_flags
  dw phdr - ehdr             ; e_ehsize
  dw _start - phdr           ; e_phentsize
  dw 1                       ; e_phnum
  dw 0                       ; e_shentsize
  dw 0                       ; e_shnum
  dw 0                       ; e_shstrndx

phdr: ; Elf32_Phdr
  dd 1           ; p_type
  dd 0           ; p_offset
  dd ehdr        ; p_vaddr
  dd ehdr        ; p_paddr
  dd _end - ehdr ; p_filesz
  dd _end - ehdr ; p_memsz
  dd 5           ; p_flags
  dd 0x1000      ; p_align

_start:
  mov bl, 42
  xor eax, eax
  inc eax
  int 0x80
_end:
