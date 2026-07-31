.code

EXTERN g_ssn : DWORD
EXTERN g_syscall_gadget : QWORD

ExecuteIndirectSyscall PROC
    ; --- JUNK CODE BLOCK 1 (Junk Register Manipulations) ---
    push rax
    push r11
    xor rax, rax
    add rax, 01337h
    sub rax, 01337h
    pop r11
    pop rax
    ; --------------------------------------------------------

    ; Orijinal İşlem: Windows x64 ABI gereği ilk parametre (cInputs) R10'a alınır
    mov r10, rcx
    
    ; --- JUNK CODE BLOCK 2 (Control Flow Flattening / Dead Branching) ---
    test r10, r10
    jnz VALID_EXEC
    nop
    nop
    xchg r11, r11

VALID_EXEC:
    ; SSN numarası EAX'e yüklenir
    mov eax, g_ssn
    
    ; --- JUNK CODE BLOCK 3 ---
    nop
    nop
    ; -------------------------

    ; win32u.dll içerisindeki "syscall; ret" gadget adresine zıpla
    jmp qword ptr [g_syscall_gadget]
ExecuteIndirectSyscall ENDP

END
