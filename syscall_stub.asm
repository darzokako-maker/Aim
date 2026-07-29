.code

EXTERN g_ssn : DWORD
EXTERN g_syscall_gadget : QWORD

ExecuteIndirectSyscall PROC
    mov r10, rcx                    ; Windows x64 Syscall ABI: 1. parametre R10'a
    mov eax, g_ssn                  ; SSN numarası EAX yazmacına
    jmp qword ptr [g_syscall_gadget] ; win32u.dll içindeki meşru 'syscall; ret' adresine zıpla
ExecuteIndirectSyscall ENDP

END
