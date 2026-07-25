#ifndef WEGSH_H
#define WEGSH_H

#include "esp_camera.h"

// ==================== WegSh ====================
// ROV'un kendi hafif, amaca ozel goruntu isleme kutuphanesi. Tam OpenCV
// DEGIL - ESP32'nin RAM/CPU'su onu kaldiramaz (OpenCV masaustu/Linux icin
// tasarlanmis, gigabaytlarca RAM varsayar). Bunun yerine tek bir ise
// odaklanan basit bir algoritma: kamera goruntusundeki KIRMIZI izi/seridi
// (renk esiklemesiyle) bulup ROV'un o izin merkezine gore ne kadar sola/saga
// sapmis oldugunu (yaw) hesaplar.
//
// Karar burada (Kamera ESP uzerinde) hesaplanir, PC'ye sadece kucuk/ucuz bir
// sayi (yaw + gorunurluk) telemetri olarak gonderilir - ham goruntu PC'ye
// islenmek uzere gonderilmez.
//
// fb: esp_camera_fb_get() ile alinan JPEG kare (formati PIXFORMAT_JPEG olmali).
// yawOut: -1.0 (tam sola sapmis) ile +1.0 (tam saga sapmis) arasi, 0=merkezde.
// visibleOut: yeterli kirmizi piksel bulunup bulunamadigi (iz kayboldugunda
//   false doner - cagiran taraf bu durumda ileri itkiyi KESMELI, korlemesine
//   surmemeli).
// Donus degeri: analiz basariyla yapilabildiyse true (JPEG cozme/bellek
//   basarisiz olursa false - bu durumda yawOut/visibleOut guvenilir degildir).
bool wegshAnalyze(camera_fb_t *fb, float &yawOut, bool &visibleOut);

#endif // WEGSH_H
