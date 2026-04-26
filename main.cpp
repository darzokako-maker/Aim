#include <windows.h>
#include <iostream>
#include <vector>
#include <random>

// --- AYARLAR ---
const COLORREF TARGET_COLOR = RGB(255, 0, 255); // Mor (Valorant Düşman Vurgusu)
const int SCAN_ZONE = 80;   // Tarama alanı (Ekran ortasındaki 80x80 alan)
const int SMOOTHING = 5;    // Yumuşatma (Ne kadar yüksekse o kadar insansı görünür)

// Rastgele gecikme üreteci (Vanguard tespiti için kritik)
int GetRandomDelay(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

void MoveMouse(int x, int y) {
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dx = x / SMOOTHING;
    input.mi.dy = y / SMOOTHING;
    input.mi.dwExtraInfo = 0; // Yazılımsal bayrağı gizle
    SendInput(1, &input, sizeof(INPUT));
}

int main() {
    HDC hdc = GetDC(NULL); 
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Uygulama ismini gizle
    SetConsoleTitleA("Windows_Input_Service");

    std::cout << "========================================" << std::endl;
    std::cout << "   YAHYA PRIVATE GHOST AIM - v15.5      " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "[*] Mod: Mor Renk Takibi" << std::endl;
    std::cout << "[*] Aktivasyon Tusu: [ V ]" << std::endl;
    std::cout << "[+] Sistem su an gizli modda calisiyor." << std::endl;

    while (true) {
        // 'V' tuşuna basılı olup olmadığını kontrol et
        if (GetAsyncKeyState('V') & 0x8000) {
            
            // Ekranın ortasını tara (Performans için 2'şer piksel atla)
            for (int y = -SCAN_ZONE; y < SCAN_ZONE; y += 2) {
                for (int x = -SCAN_ZONE; x < SCAN_ZONE; x += 2) {
                    COLORREF color = GetPixel(hdc, (screenWidth / 2) + x, (screenHeight / 2) + y);
                    
                    // Mor renk kontrolü (Renk payı eklendi)
                    if (GetRValue(color) > 180 && GetBValue(color) > 180 && GetGValue(color) < 100) {
                        MoveMouse(x, y); 
                        
                        // Her hareketten sonra çok kısa rastgele bekleme
                        Sleep(GetRandomDelay(1, 3)); 
                        goto end_loop; 
                    }
                }
            }
        }
        
        end_loop:
        // CPU kullanımını düşürmek ve sistemi stabil tutmak için
        Sleep(1); 

        // PANIK TUSU: Klavyeden 'END' tuşuna basarsan program anında kapanır
        if (GetAsyncKeyState(VK_END)) break;
    }

    ReleaseDC(NULL, hdc);
    return 0;
}

