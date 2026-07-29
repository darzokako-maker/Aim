.code

EXTERN g_ssn : DWORD
EXTERN g_syscall_gadget : QWORD

ExecuteIndirectSyscall PROC
    mov r10, rcx                    ; Windows x64 ABI gereği ilk parametre (cInputs) R10'a alınır
    mov eax, g_ssn                  ; SSN numarası EAX yazmacına yüklenir
    jmp qword ptr [g_syscall_gadget] ; win32u.dll içindeki meşru syscall gadget adresine zıplar
ExecuteIndirectSyscall ENDP

END

