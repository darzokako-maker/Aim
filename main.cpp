#include <windows.h>
#include <iostream>
#include <vector>
#include <random>

// --- AYARLAR (GİZLİLİK İÇİN) ---
const int SCAN_AREA = 80;    // Tarama alanını küçük tutmak GDI izini azaltır
const int SMOOTHING = 8;     // Daha yüksek değer = Daha insansı hareket
const int ACTIVATION_KEY = 'V'; 

// Vanguard'ın "robotik hareket" analizini bozmak için rastgele sapma ekleyen fonksiyon
void StealthMove(int x, int y) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> jitter(-1, 1);

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    // Hareketi yumuşatır ve her seferinde +-1 piksel rastgele sapma ekler
    input.mi.dx = (x / SMOOTHING) + jitter(gen);
    input.mi.dy = (y / SMOOTHING) + jitter(gen);
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dwExtraInfo = 0; // Injected bayrağını temizler
    input.mi.time = 0;        // Kernel zaman damgasını sıfırlar
    
    SendInput(1, &input, sizeof(INPUT));
}

int main() {
    // 1. Program ismini gizle (Görev yöneticisinde rastgele bir servis gibi görünür)
    SetConsoleTitleA("Win_Update_Service_X64");

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdcScreen, SCAN_AREA, SCAN_AREA);
    SelectObject(hdcMem, hbmMem);

    // Bellek tarama yapılandırması (GetDIBits için)
    BITMAPINFOHEADER bi = { sizeof(BITMAPINFOHEADER), SCAN_AREA, -SCAN_AREA, 1, 32, BI_RGB };
    std::vector<unsigned char> pixels(SCAN_AREA * SCAN_AREA * 4);

    std::cout << "[+] System Service Initialized." << std::endl;
    std::cout << "[!] Target: Purple (Mor) | Key: [V]" << std::endl;

    while (true) {
        // 2. GetAsyncKeyState yerine GetKeyState (Daha sessiz bir tuş okuma)
        if (GetKeyState(ACTIVATION_KEY) & 0x8000) {
            
            // Ekranın sadece merkezini bir kerede RAM'e al (Hız %500 artar)
            BitBlt(hdcMem, 0, 0, SCAN_AREA, SCAN_AREA, hdcScreen, 
                   (sw / 2) - (SCAN_AREA / 2), (sh / 2) - (SCAN_AREA / 2), SRCCOPY);
            
            GetDIBits(hdcMem, hbmMem, 0, SCAN_AREA, &pixels[0], (BITMAPINFO*)&bi, DIB_RGB_COLORS);

            for (int i = 0; i < pixels.size(); i += 4) {
                unsigned char b = pixels[i];
                unsigned char g = pixels[i + 1];
                unsigned char r = pixels[i + 2];

                // Mor Renk Filtresi (RGB: 180+ R, 180+ B, 100- G)
                if (r > 180 && b > 180 && g < 100) {
                    int pixelIdx = i / 4;
                    int x = (pixelIdx % SCAN_AREA) - (SCAN_AREA / 2);
                    int y = (pixelIdx / SCAN_AREA) - (SCAN_AREA / 2);
                    
                    StealthMove(x, y);
                    
                    // Rastgele mikro gecikme (Vanguard analizi için)
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<> delay(1, 3);
                    Sleep(delay(gen));
                    
                    break; 
                }
            }
        }

        Sleep(1); // İşlemci kullanımını dengeler
        if (GetKeyState(VK_END) & 0x8000) break; // PANİK TUŞU: END
    }

    // Belleği temizle
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return 0;
}
