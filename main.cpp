#include <windows.h>
#include <iostream>
#include <vector>
#include <random>

// --- AYARLAR (GİZLİLİK İÇİN DEĞİŞTİREBİLİRSİN) ---
const int AREA = 100;       // Tarama alanı (100x100)
const int SMOOTHING = 6;    // Yumuşatma (Artırırsan daha insansı olur)
const int TARGET_V_KEY = 'V'; 

// Fare hareketini sisteme "doğal" olarak ileten fonksiyon
void StealthMove(int x, int y) {
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dx = x / SMOOTHING;
    input.mi.dy = y / SMOOTHING;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dwExtraInfo = 0; // Injected bayrağını temizle
    input.mi.time = 0;
    SendInput(1, &input, sizeof(INPUT));
}

int main() {
    // Performans için ekran bilgilerini bir kez al
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdcScreen, AREA, AREA);
    SelectObject(hdcMem, hbmMem);

    // Bellek taraması için Bitmap yapılandırması
    BITMAPINFOHEADER bi = { sizeof(BITMAPINFOHEADER), AREA, -AREA, 1, 32, BI_RGB };
    std::vector<unsigned char> pixels(AREA * AREA * 4);

    // Konsol Gizleme ve Başlık
    SetConsoleTitleA("System_Host_Service"); 
    std::cout << "--- YAHYA PRIVATE STEALTH v18.2 ---" << std::endl;
    std::cout << "[+] Mod: RAM-Based Memory Scanning" << std::endl;
    std::cout << "[+] Kontrol Tusu: [ V ]" << std::endl;

    while (true) {
        // V tuşuna basıldığında aktifleş
        if (GetAsyncKeyState(TARGET_V_KEY) & 0x8000) {
            
            // 1. Ekranın ortasını anlık olarak belleğe (RAM) kopyala
            BitBlt(hdcMem, 0, 0, AREA, AREA, hdcScreen, (sw / 2) - (AREA / 2), (sh / 2) - (AREA / 2), SRCCOPY);
            
            // 2. Bellekteki verileri diziye aktar
            GetDIBits(hdcMem, hbmMem, 0, AREA, &pixels[0], (BITMAPINFO*)&bi, DIB_RGB_COLORS);

            // 3. Bellek üzerinde mor renk araması yap
            for (int i = 0; i < pixels.size(); i += 4) {
                unsigned char b = pixels[i];
                unsigned char g = pixels[i + 1];
                unsigned char r = pixels[i + 2];

                // Mor Renk Filtresi (Valorant Vurgusu)
                if (r > 180 && b > 180 && g < 100) {
                    int pixelIdx = i / 4;
                    int x = (pixelIdx % AREA) - (AREA / 2);
                    int y = (pixelIdx / AREA) - (AREA / 2);
                    
                    StealthMove(x, y);
                    
                    // Vanguard'ın ritim analizini bozmak için mikro bekleme
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<> d(1, 3);
                    Sleep(d(gen));
                    
                    break; // Hedef bulundu, bir sonraki kareye geç
                }
            }
        }

        // CPU yorulmasın ve stabilite sağlansın
        Sleep(1); 

        // Acil kapatma (END tuşu)
        if (GetAsyncKeyState(VK_END)) break;
    }

    // Bellek temizliği
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return 0;
}
