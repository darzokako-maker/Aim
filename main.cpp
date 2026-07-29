#include <windows.h>
#include <iostream>
#include <vector>
#include <random>
#include <cmath>

// --- SYSCALL DEĞİŞKENLERİ VE FONKSİYON BİLDİRİMİ ---
extern "C" {
    DWORD g_ssn = 0;
    ULONG_PTR g_syscall_gadget = 0;
    UINT ExecuteIndirectSyscall(UINT cInputs, LPINPUT pInputs, int cbSize);
}

// --- AYARLAR ---
const int SCAN_AREA = 80;    
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

// Gauss (Normal) dağılım ile insansı milisaniye gecikmesi üreten yardımcı fonksiyon
int GetGaussianDelay(double mean, double stddev) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> dist(mean, stddev);
    int delay = static_cast<int>(std::round(dist(gen)));
    return delay < 1 ? 1 : delay;
}

// İnsansı, eğrisel (Bezier) ve değişken hızlı fare hareketi (Indirect Syscall destekli)
void StealthMove(int targetX, int targetY) {
    if (targetX == 0 && targetY == 0) return;

    double distance = std::sqrt(targetX * targetX + targetY * targetY);
    
    int steps = static_cast<int>(distance / 3.0);
    if (steps < 5) steps = 5;       
    if (steps > 50) steps = 50;     

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> curveOffset(-0.2, 0.2);
    
    double ctrlX = targetX * 0.5 + (targetY * curveOffset(gen));
    double ctrlY = targetY * 0.5 - (targetX * curveOffset(gen));

    double lastActualX = 0;
    double lastActualY = 0;

    for (int i = 1; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        double easeT = 1.0 - std::pow(1.0 - t, 3.0);

        double currentX = std::pow(1.0 - easeT, 2.0) * 0 + 2.0 * (1.0 - easeT) * easeT * ctrlX + std::pow(easeT, 2.0) * targetX;
        double currentY = std::pow(1.0 - easeT, 2.0) * 0 + 2.0 * (1.0 - easeT) * easeT * ctrlY + std::pow(easeT, 2.0) * targetY;

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

        int stepDelay = GetGaussianDelay(2.0, 0.5);
        Sleep(stepDelay);
    }
}

int main() {
    SetConsoleTitleA("Win_Update_Service_X64");

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

                if (r > 180 && b > 180 && g < 100) {
                    int pixelIdx = static_cast<int>(i / 4);
                    int x = (pixelIdx % SCAN_AREA) - (SCAN_AREA / 2);
                    int y = (pixelIdx / SCAN_AREA) - (SCAN_AREA / 2);
                    
                    StealthMove(x, y);
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
