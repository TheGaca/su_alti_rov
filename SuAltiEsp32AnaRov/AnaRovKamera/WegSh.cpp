#include "WegSh.h"
#include <Arduino.h>         // ps_malloc() burada tanimli - .ino dosyalarinin aksine .cpp dosyalarina otomatik eklenmez
#include "img_converters.h" // fmt2rgb888() - esp32-camera kutuphanesinin parcasi

// ==================== Ayarlanabilir sabitler ====================
// Gercek havuz/tank isiginda bunlarin ince ayar gerektirmesi beklenir - ilk
// canli testten sonra birlikte kalibre edilecek, "bir kere yazinca biter"
// degil.
#define WEGSH_GRID_STEP      8   // her N pikselde bir ornekle (CPU tasarrufu icin tum pikselleri taramiyoruz)
#define WEGSH_RED_MIN        120 // R kanali en az bu kadar olmali (0-255)
#define WEGSH_RED_RATIO_PCT  130 // R, G ve B'nin en az bu/100 kati olmali (orn. 130 = 1.3x) - HSV donusumunden ucuz bir yaklasim
#define WEGSH_MIN_RED_PIXELS 40  // bundan az kirmizi ornek noktasi bulunursa "gorunmuyor" sayilir

bool wegshAnalyze(camera_fb_t *fb, float &yawOut, bool &visibleOut) {
    yawOut = 0.0f;
    visibleOut = false;

    if (fb == nullptr || fb->format != PIXFORMAT_JPEG || fb->width == 0 || fb->height == 0) {
        return false;
    }

    // NOT (v1 performans notu): burada tum kareyi RGB888'e coz, sonra seyrek
    // bir izgarayla ornekliyoruz - JPEG'in kismi/olcekli cozulmesi (sadece
    // ihtiyac duyulan cozunurlukte) daha verimli olurdu ama esp32-camera'nin
    // temel fmt2rgb888() API'si bunu desteklemiyor. Ilk calisan surum icin
    // yeterli; gercek donanimda hiz sorun olursa optimize ederiz.
    size_t rgbLen = (size_t)fb->width * (size_t)fb->height * 3;
    uint8_t *rgbBuf = (uint8_t *)ps_malloc(rgbLen); // PSRAM'dan ayir - bu boyut normal heap'i tasirir
    if (rgbBuf == nullptr) {
        return false; // bellek yetersiz - guvenli sekilde vazgec, bir sonraki turda tekrar denenir
    }

    bool decoded = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, rgbBuf);
    if (!decoded) {
        free(rgbBuf);
        return false;
    }

    long sumX = 0;
    long redCount = 0;

    for (int y = 0; y < (int)fb->height; y += WEGSH_GRID_STEP) {
        for (int x = 0; x < (int)fb->width; x += WEGSH_GRID_STEP) {
            size_t idx = ((size_t)y * fb->width + x) * 3;
            uint8_t r = rgbBuf[idx];
            uint8_t g = rgbBuf[idx + 1];
            uint8_t b = rgbBuf[idx + 2];
            // Basit RGB esigi: "kirmizi", G ve B'ye gore belirgin sekilde
            // yuksek R demektir. Tam HSV donusumunden cok daha ucuz, ESP32
            // icin yeterli.
            if (r >= WEGSH_RED_MIN &&
                (int)r * 100 >= (int)g * WEGSH_RED_RATIO_PCT &&
                (int)r * 100 >= (int)b * WEGSH_RED_RATIO_PCT) {
                sumX += x;
                redCount++;
            }
        }
    }

    free(rgbBuf);

    if (redCount < WEGSH_MIN_RED_PIXELS) {
        visibleOut = false;
        yawOut = 0.0f;
        return true;
    }

    float centroidX = (float)sumX / (float)redCount;
    float halfWidth = fb->width / 2.0f;
    yawOut = (centroidX - halfWidth) / halfWidth; // -1..1, negatif=sola sapmis, pozitif=saga sapmis
    if (yawOut < -1.0f) yawOut = -1.0f;
    if (yawOut > 1.0f) yawOut = 1.0f;
    visibleOut = true;
    return true;
}
