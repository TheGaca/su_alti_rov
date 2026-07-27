#ifndef MOTORMIXER_H
#define MOTORMIXER_H

#include <array>

// ESP32 8 motorlu ROV govdesinin motor karisimi - joystick/otonom girdilerini
// (surge, lateral, yaw, vertical, -1..1) 8 motorun PWM darbe genisligine (us)
// cevirir. Bilerek Qt'siz tutuldu: saf C++ oldugu icin GUI'den bagimsiz birim
// testlenebilir (bkz. tests/test_motor_mixer.cpp).
//
// M1..M4 = on/arka capraz yatay itki, M5..M8 = dikey itki
// (bkz. foto/ dizinindeki motor semasi ve MotorDiagramWidget). Isaretler ilk
// tahmindir: bench testinde bir motor beklenenin tersine donerse ilgili
// agirligi (-1 <-> +1) ters cevirmek yeterlidir.
namespace MotorMixer {

constexpr int NEUTRAL_US = 1490;
constexpr int DELTA_US   = 200;

// ESC'ye giden darbenin PC tarafindaki sert tavan/tabani - ESP32 firmware'indeki
// (AnaRovBeyin.ino / sualtiesp.ino) MIN_US/MAX_US ile ayni degerler; motor
// hizini iki katmanda da tutarli sekilde sinirlar.
constexpr int MIN_US = 1295;
constexpr int MAX_US = 1600;

// surge (ileri+), lateral (sag+), yaw (saga don+), vertical (yukari+) -> 8 darbe.
// rollCorr/pitchCorr: IMU stabilize duzeltmeleri (-1..1), sadece dikey motorlara
// (M5-M8) eklenir; 0 gecilirse dort dikey motor ayni degeri alir.
std::array<int, 8> compute(float surge, float lateral, float yaw, float vertical,
                           float rollCorr = 0.0f, float pitchCorr = 0.0f);

} // namespace MotorMixer

#endif // MOTORMIXER_H
