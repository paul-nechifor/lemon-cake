BITS 64
; org 0x08048000 ???

ehdr: ; Elf64_Ehdr
  db 0x7F, "ELF", 2, 1, 1, 0 ; e_ident
  dq 0
  dw 2                       ; e_type
  dw 0x3E                    ; e_machine
  dd 1                       ; e_version
  dq 0x4000b0                ; e_entry
  dq 0x40                    ; e_phoff
  dq 0xe0                    ; e_shoff
  dd 0                       ; e_flags
  dw 0x40                    ; e_ehsize
  dw 0x38                    ; e_phentsize
  dw 2                       ; e_phnum
  dw 0x40                    ; e_shentsize
  dw 4                       ; e_shnum
  dw 3                       ; e_shstrndx

phdr: ; Elf64_Phdr
  dd 1           ; p_type
  dd 5           ; p_flags
  dq 0           ; p_offset
  dq 0x400000    ; p_vaddr
  dq 0x400000    ; p_paddr
  dq 0xbc        ; p_filesz
  dq 0xbc        ; p_memsz
  dq 0x200000    ; p_align

  dd 1           ; p_type
  dd 6           ; p_flags
  dq 0xbc        ; p_offset
  dq 0x6000bc    ; p_vaddr
  dq 0x6000bc    ; p_paddr
  dq 8           ; p_filesz
  dq 8           ; p_memsz
  dq 0x200000    ; p_align



  mov rax, 60
  mov rdi, 42
  syscall

  db 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01

  db 0x00
  db 0x2e, 0x73, 0x68, 0x73, 0x74, 0x72, 0x74, 0x61, 0x62, 0x00
  db 0x2e, 0x74, 0x65, 0x78, 0x74, 0x00
  db 0x2e, 0x64, 0x61, 0x74, 0x61, 0x00
  db 0x00, 0x00, 0x00, 0x00, 0x00




  dd 0 ; sh_name
  dd 0 ; sh_type
  dq 0 ; sh_flags
  dq 0 ; sh_addr
  dq 0 ; sh_offset
  dq 0 ; sh_size
  dd 0 ; sh_link
  dd 0 ; sh_info
  dq 0 ; sh_addralign
  dq 0 ; sh_entsize

  dd 0x0b ; sh_name
  dd 1 ; sh_type
  dq 0x06 ; sh_flags
  dq 0x4000b0 ; sh_addr
  dq 0xb0 ; sh_offset
  dq 0x0c ; sh_size
  dd 0 ; sh_link
  dd 0 ; sh_info
  dq 0x10 ; sh_addralign
  dq 0 ; sh_entsize

  dd 0x11 ; sh_name
  dd 1 ; sh_type
  dq 0x03 ; sh_flags
  dq 0x6000bc ; sh_addr
  dq 0xbc ; sh_offset
  dq 0x08 ; sh_size
  dd 0 ; sh_link
  dd 0 ; sh_info
  dq 4 ; sh_addralign
  dq 0 ; sh_entsize

  dd 0x01 ; sh_name
  dd 3 ; sh_type
  dq 0x00 ; sh_flags
  dq 0 ; sh_addr
  dq 0xc4 ; sh_offset
  dq 0x17 ; sh_size
  dd 0 ; sh_link
  dd 0 ; sh_info
  dq 1 ; sh_addralign
  dq 0 ; sh_entsize
