format ELF
use32

public _start


section '.text' executable

_start:
mov     esp,0xffffe000

mov     eax,192     ; mmap2
mov     ebx,4096    ; address
mov     ecx,0x07fff000 ; size
mov     edx,0x7     ; PROT_READ | PROT_WRITE | PROT_EXEC
mov     esi,0x11    ; MAP_SHARED | MAP_FIXED
mov     edi,15      ; file descriptor
mov     ebp,1       ; offset
int     0x80


; segfault for alerting hypervisor
cli


; force whole page to be RWX

section '.bss' writable align 2048

times 2048 db ?
