.code

EXTERN g_ssn : DWORD
EXTERN g_syscall_gadget : QWORD

ExecuteIndirectSyscall PROC
    ; Windows x64 ABI: İlk parametre (cInputs) R10'a taşınır
    mov r10, rcx
    
    ; SSN numarası EAX'e yüklenir
    mov eax, g_ssn
    
    ; win32u.dll içerisindeki "syscall; ret" adresine zıpla
    jmp qword ptr [g_syscall_gadget]
ExecuteIndirectSyscall ENDP

END
