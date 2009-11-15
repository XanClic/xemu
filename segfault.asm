use32

format ELF

public segfault_handler
extrn _c_segfault_handler

section '.text' executable

segfault_handler:
push  esp
call  _c_segfault_handler
add   esp,4
ret
