.code

IssueSyscall PROC
    syscall
    ret
    hlt     ; Unreachable.
IssueSyscall ENDP

CallRax PROC
    call rax
    ret
    hlt     ; Unreachable.
CallRax ENDP

Halt PROC
    hlt     ; __noreturn
Halt ENDP

InterruptThree PROC
    int 3
InterruptThree ENDP

ForceComparison_Number PROC
    test rcx, rdx
ForceComparison_Number ENDP

GetFlags PROC
    pushfq
    pop rax
    ret
GetFlags ENDP

PushRip PROC
    mov rax, qword ptr [rsp + 8h]
PushRip ENDP

END