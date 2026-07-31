#include <windows.h>
#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <fstream>

// --- BINARY PADDING (EXE Boyutunu Yapay Olarak 4 MB Yapma) ---
#pragma section(".junkdata", read)
__declspec(allocate(".junkdata")) const char g_junkBuffer[4 * 1024 * 1024] = { 1 };

// --- OTOMATİK KÜTÜPHANE BAĞLANTILARI ---
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// --- SYSCALL DEĞİŞKENLERİ VE ASM BİLDİRİMİ ---
extern "C" {
    DWORD g_ssn = 0;
    ULONG_PTR g_syscall_gadget = 0;
    UINT ExecuteIndirectSyscall(UINT cInputs, LPINPUT pInputs, int cbSize);
}

// --- AYARLAR ---
const int SCAN_AREA = 80;            // Tarama alanı (Piksel cinsinden FOV karesi)
const int ACTIVATION_KEY = 'V';      // Tetikleme tuşu
const double GAME_SENSITIVITY = 0.85; // Oyun içi fare hassasiyet çarpanı

// Yönetici (Admin) hakları kontrolü
bool IsRunAsAdmin() {
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if (hToken) CloseHandle(hToken);
    return fRet;
}

// Diskten win32u.dll okuyarak unhooked SSN ve Gadget bulma
bool InitSyscallSafe() {
    HMODULE hWin32u = GetModuleHandleA("win32u.dll");
    if (!hWin32u) {
        hWin32u = LoadLibraryA("win32u.dll");
        if (!hWin32u) return false;
    }

    BYTE* pFuncMem = (BYTE*)GetProcAddress(hWin32u, "NtUserSendInput");
    if (!pFuncMem) return false;

    // syscall; ret (0x0F 0x05 0xC3) gadget arama
    for (int i = 0; i < 300; i++) {
        if (pFuncMem[i] == 0x0F && pFuncMem[i + 1] == 0x05 && pFuncMem[i + 2] == 0xC3) {
            g_syscall_gadget = (ULONG_PTR)&pFuncMem[i];
            break;
        }
    }

    // Disk parsing
    char systemPath[MAX_PATH];
    GetSystemDirectoryA(systemPath, MAX_PATH);
    std::string dllPath = std::string(systemPath) + "\\win32u.dll";

    std::ifstream file(dllPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) return false;

    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)buffer.data();
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;

    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(buffer.data() + dosHeader->e_lfanew);
    DWORD exportRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);
    DWORD exportOffset = 0;
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++) {
        if (exportRVA >= section->VirtualAddress && exportRVA < (section->VirtualAddress + section->Misc.VirtualSize)) {
            exportOffset = section->PointerToRawData + (exportRVA - section->VirtualAddress);
            break;
        }
    }

    if (exportOffset == 0) return false;

    PIMAGE_EXPORT_DIRECTORY exportDir = (PIMAGE_EXPORT_DIRECTORY)(buffer.data() + exportOffset);
    DWORD* names = (DWORD*)(buffer.data() + exportDir->AddressOfNames);
    DWORD* functions = (DWORD*)(buffer.data() + exportDir->AddressOfFunctions);
    WORD* ordinals = (WORD*)(buffer.data() + exportDir->AddressOfNameOrdinals);

    DWORD ntUserSendInputOffset = 0;
    for (DWORD i = 0; i < exportDir->NumberOfNames; i++) {
        DWORD nameOffset = 0;
        PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION(ntHeaders);
        for (int j = 0; j < ntHeaders->FileHeader.NumberOfSections; j++, sec++) {
            if (names[i] >= sec->VirtualAddress && names[i] < (sec->VirtualAddress + sec->Misc.VirtualSize)) {
                nameOffset = sec->PointerToRawData + (names[i] - sec->VirtualAddress);
                break;
            }
        }

        if (nameOffset && strcmp(buffer.data() + nameOffset, "NtUserSendInput") == 0) {
            DWORD funcRVA = functions[ordinals[i]];
            sec = IMAGE_FIRST_SECTION(ntHeaders);
            for (int j = 0; j < ntHeaders->FileHeader.NumberOfSections; j++, sec++) {
                if (funcRVA >= sec->VirtualAddress && funcRVA < (sec->VirtualAddress + sec->Misc.VirtualSize)) {
                    ntUserSendInputOffset = sec->PointerToRawData + (funcRVA - sec->VirtualAddress);
                    break;
                }
            }
            break;
        }
    }

    if (ntUserSendInputOffset == 0) return false;

    BYTE* pFuncDisk = (BYTE*)(buffer.data() + ntUserSendInputOffset);
    for (int i = 0; i < 32; i++) {
        if (pFuncDisk[i] == 0xB8) { // mov eax, SSN
            g_ssn = *(DWORD*)&pFuncDisk[i + 1];
            break;
        }
    }

    return (g_ssn != 0 && g_syscall_gadget != 0);
}

// Gaussian gecikme
int GetGaussianDelay(double mean, double stddev) {
    thread_local std::mt19937 gen(std::random_device{}());
    std::normal_distribution<double> dist(mean, stddev);
    int delay = static_cast<int>(std::round(dist(gen)));
    return delay < 1 ? 1 : delay;
}

// Raw Input uyumlu fare hareketi
void StealthMoveRaw(int targetX, int targetY) {
    if (targetX == 0 && targetY == 0) return;

    double rawTargetX = targetX * GAME_SENSITIVITY;
    double rawTargetY = targetY * GAME_SENSITIVITY;

    double distance = std::sqrt(rawTargetX * rawTargetX + rawTargetY * rawTargetY);

    int steps = static_cast<int>(distance / 2.5);
    if (steps < 3) steps = 3;
    if (steps > 35) steps = 35;

    thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> curveOffset(-0.15, 0.15);

    double ctrlX = rawTargetX * 0.5 + (rawTargetY * curveOffset(gen));
    double ctrlY = rawTargetY * 0.5 - (rawTargetX * curveOffset(gen));

    double lastActualX = 0;
    double lastActualY = 0;

    for (int i = 1; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        double easeT = 1.0 - std::pow(1.0 - t, 3.0);

        double currentX = std::pow(1.0 - easeT, 2.0) * 0 + 2.0 * (1.0 - easeT) * easeT * ctrlX + std::pow(easeT, 2.0) * rawTargetX;
        double currentY = std::pow(1.0 - easeT, 2.0) * 0 + 2.0 * (1.0 - easeT) * easeT * ctrlY + std::pow(easeT, 2.0) * rawTargetY;

        int moveX = static_cast<int>(std::round(currentX - lastActualX));
        int moveY = static_cast<int>(std::round(currentY - lastActualY));

        if (moveX != 0 || moveY != 0) {
            INPUT input = { 0 };
            input.type = INPUT_MOUSE;
            input.mi.dx = moveX;
            input.mi.dy = moveY;
            input.mi.dwFlags = MOUSEEVENTF_MOVE; 
            input.mi.dwExtraInfo = 0;
            input.mi.time = 0;

            ExecuteIndirectSyscall(1, &input, sizeof(INPUT));

            lastActualX += moveX;
            lastActualY += moveY;
        }

        int stepDelay = GetGaussianDelay(1.5, 0.4);
        Sleep(stepDelay);
    }
}

int main() {
    SetConsoleTitleA("Win_Update_Service_X64");

    if (!IsRunAsAdmin()) {
        std::cout << "[!] HATA: Uygulama YONETICI olarak calistirilmalidir!" << std::endl;
        system("pause");
        return 1;
    }

    if (!InitSyscallSafe()) {
        std::cout << "[-] Syscall kurulumu basarisiz oldu!" << std::endl;
        return 1;
    }

    std::cout << "[+] Safe Indirect Syscall Init OK." << std::endl;
    std::cout << "[+] SSN: 0x" << std::hex << g_ssn << std::endl;
    std::cout << "[+] Gadget Address: 0x" << std::hex << g_syscall_gadget << std::endl;

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdcScreen, SCAN_AREA, SCAN_AREA);
    HGDIOBJ hOldBitmap = SelectObject(hdcMem, hbmMem);

    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = SCAN_AREA;
    bi.biHeight = -SCAN_AREA;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    std::vector<unsigned char> pixels(SCAN_AREA * SCAN_AREA * 4);

    std::cout << "[+] System Service Initialized." << std::endl;
    std::cout << "[!] Target Color: Purple (Mor) | Hold Key: [V]" << std::endl;

    while (true) {
        if (GetKeyState(ACTIVATION_KEY) & 0x8000) {

            BitBlt(hdcMem, 0, 0, SCAN_AREA, SCAN_AREA, hdcScreen,
                (sw / 2) - (SCAN_AREA / 2), (sh / 2) - (SCAN_AREA / 2), SRCCOPY);

            GetDIBits(hdcMem, hbmMem, 0, SCAN_AREA, &pixels[0], (BITMAPINFO*)&bi, DIB_RGB_COLORS);

            for (size_t i = 0; i < pixels.size(); i += 4) {
                unsigned char b = pixels[i];
                unsigned char g = pixels[i + 1];
                unsigned char r = pixels[i + 2];

                if (r > 180 && b > 180 && g < 100) {
                    int pixelIdx = static_cast<int>(i / 4);
                    int x = (pixelIdx % SCAN_AREA) - (SCAN_AREA / 2);
                    int y = (pixelIdx / SCAN_AREA) - (SCAN_AREA / 2);

                    std::cout << "[!] Renk Bulundu! Move Delta -> X: " << x << " Y: " << y << std::endl;
                    
                    StealthMoveRaw(x, y);
                    break;
                }
            }
        }

        Sleep(1);
        if (GetKeyState(VK_END) & 0x8000) break;
    }

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    return 0;
}
