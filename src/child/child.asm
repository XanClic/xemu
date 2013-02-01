format ELF
use32

public _start


section '.text' executable

_start:
mov     esp,0xffffd000


mov     eax,192     ; mmap2
mov     ebx,esp     ; address
mov     ecx,4096    ; size
mov     edx,0x3     ; PROT_READ | PROT_WRITE
mov     esi,0x11    ; MAP_SHARED | MAP_FIXED
mov     edi,14      ; file descriptor
mov     ebp,0       ; offset
int     0x80


mov     eax,192     ; mmap2
mov     ebx,4096    ; address
mov     ecx,0x07fff000 ; size
mov     edx,0x7     ; PROT_READ | PROT_WRITE | PROT_EXEC
mov     esi,0x11    ; MAP_SHARED | MAP_FIXED
mov     edi,15      ; file descriptor
mov     ebp,1       ; offset (pages)
int     0x80


; save entry
mov     eax,[0xffffd000]
mov     [entry_point],eax


; the following code is practically VM code and must therefore be executed in
; shared memory.

mov     esi,0xffffc000
mov     edi,0x000f0000
mov     ecx,1024
rep     movsd

jmp     low+0x000f4000
low:

mov     dword [0x1008],0x0000ffff
mov     dword [0x100c],0x00cf9a00
mov     dword [0x1010],0x0000ffff
mov     dword [0x1014],0x00cf9200


mov     word  [gdtr+0x000f4000],0x17
mov     dword [gdtr+0x000f4002],0x1000


lgdt    [gdtr+0x000f4000]


jmp far 0x08:load_cs+0x000f4000
load_cs:

mov     ax,0x10
mov     ds,ax
mov     es,ax
mov     fs,ax
mov     gs,ax
mov     ss,ax


; multiboot information
mov     eax,0x2badb002
mov     ebx,0x00010000


mov     edx,[entry_point+0x000f4000]
jmp     edx


; force whole page to be RWX

section '.bss' writable align 2048

entry_point dd ?
gdtr:
dw ?
dd ?

times 2038 db ?
