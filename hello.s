section .text
global _start
_start:
    mov     edx,msg.len                         ;message length
    mov     ecx,msg                             ;message to write
    mov     ebx,1                               ;file descriptor (stdout)
    mov     eax,4                               ;system call number (sys_write)
    int     0x80                                ;call kernel
section .data
msg     db  'Hello, world!',0xa                 ;our dear string
msg.len equ $ - msg                             ;length of our dear string
section .text
    mov     eax,1
    int     0x80
