[BITS 32]

; exit(int status)
; 系统调用号: 0
global exit
exit:
    mov eax, 0          ; sys_exit
    mov ebx, [esp+4]    ; 参数1: status
    int 0x80
    ret                 ; 不应该返回，但以防万一

; fork()
; 系统调用号: 1
global fork
fork:
    mov eax, 1          ; sys_fork
    int 0x80
    ret                 ; 返回值在eax中

; waitpid(int pid)
; 系统调用号: 2
global waitpid
waitpid:
    mov eax, 2          ; sys_waitpid
    mov ebx, [esp+4]    ; 参数1: pid
    int 0x80
    ret                 ; 返回值在eax中

; exec(const char* filename)
; 系统调用号: 3
global exec
exec:
    mov eax, 3          ; sys_exec
    mov ebx, [esp+4]    ; 参数1: filename
    int 0x80
    ret                 ; 返回值在eax中
