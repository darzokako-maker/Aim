#include <windows.h>
#include <iostream>
#include <vector>
#include <random>

// --- SYSCALL DEĞİŞKENLERİ VE FONKSİYON BİLDİRİMİ ---
extern "C" {
    DWORD g_ssn = 0;
    ULONG_PTR g_syscall_gadget = 0;
    UINT ExecuteIndirectSyscall(UINT cInputs, LPINPUT pInputs, int cbSize);
}

// --- AYARLAR ---
const int SCAN_AREA = 80;    
const int SMOOTHING = 8;     
const int ACTIVATION_KEY = 'V'; 

// win32u.dll içerisinden NtUserSendInput'a ait SSN ve Syscall adresini çözen fonksiyon
bool InitSyscall() {
    HMODULE hWin32u = LoadLibraryA("win32u.dll");
    if (!hWin32u) return false;

    BYTE* pFunc = (BYTE*)GetProcAddress(hWin32u, "NtUserSendInput");
    if (!pFunc) return false;

    // 1. SSN Numarasını Ayrıştır (mov eax, SSN -> 0xB8 opkodu)
    for (int i = 0; i < 32; i++) {
        if (pFunc[i] == 0xB8) {
            g_ssn = *(DWORD*)&pFunc[i + 1];
            break;
        }
    }

    // 2. win32u.dll içindeki "syscall; ret" (0x0F 0x05) adresini bul
    for (int i = 0; i < 64; i++) {
        if (pFunc[i] == 0x0F && pFunc[i + 1] == 0x05) {
            g_syscall_gadget = (ULONG_PTR)&pFunc[i];
            break;
        }
    }

    return (g_ssn != 0 && g_syscall_gadget != 0);
}

// Standart SendInput yerine Indirect Syscall kullanan hareket fonksiyonu
void StealthMove(int x, int y) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> jitter(-1, 1);

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dx = (x / SMOOTHING) + jitter(gen);
    input.mi.dy = (y / SMOOTHING) + jitter(gen);
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dwExtraInfo = 0;
    input.mi.time = 0;
    
    // Standart SendInput yerine Indirect Syscall çağrısı yapılır
    ExecuteIndirectSyscall(1, &input, sizeof(INPUT));
}

int main() {
    SetConsoleTitleA("Win_Update_Service_X64");

    // Syscall tablosunu ve gadget adreslerini hazırlar
    if (!InitSyscall()) {
        std::cout << "[-] Syscall kurulumu basarisiz oldu!" << std::endl;
        return 1;
    }

    std::cout << "[+] Syscall Init OK. SSN: 0x" << std::hex << g_ssn << std::endl;

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdcScreen, SCAN_AREA, SCAN_AREA);
    SelectObject(hdcMem, hbmMem);

    BITMAPINFOHEADER bi = { sizeof(BITMAPINFOHEADER), SCAN_AREA, -SCAN_AREA, 1, 32, BI_RGB };
    std::vector<unsigned char> pixels(SCAN_AREA * SCAN_AREA * 4);

    std::cout << "[+] System Service Initialized." << std::endl;
    std::cout << "[!] Target: Purple (Mor) | Key: [V]" << std::endl;

    while (true) {
        if (GetKeyState(ACTIVATION_KEY) & 0x8000) {
            
            BitBlt(hdcMem, 0, 0, SCAN_AREA, SCAN_AREA, hdcScreen, 
                   (sw / 2) - (SCAN_AREA / 2), (sh / 2) - (SCAN_AREA / 2), SRCCOPY);
            
            GetDIBits(hdcMem, hbmMem, 0, SCAN_AREA, &pixels[0], (BITMAPINFO*)&bi, DIB_RGB_COLORS);

            for (size_t i = 0; i < pixels.size(); i += 4) {
                unsigned char b = pixels[i];
                unsigned char g = pixels[i + 1];
                unsigned char r = pixels[i + 2];

                // Mor Renk Filtresi
                if (r > 180 && b > 180 && g < 100) {
                    int pixelIdx = static_cast<int>(i / 4);
                    int x = (pixelIdx % SCAN_AREA) - (SCAN_AREA / 2);
                    int y = (pixelIdx / SCAN_AREA) - (SCAN_AREA / 2);
                    
                    StealthMove(x, y);
                    
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<> delay(1, 3);
                    Sleep(delay(gen));
                    
                    break; 
                }
            }
        }

        Sleep(1);
        if (GetKeyState(VK_END) & 0x8000) break;
    }

    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return 0;
}
