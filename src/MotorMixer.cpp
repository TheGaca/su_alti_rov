#include "MotorMixer.hpp"

#include <algorithm>

namespace MotorMixer {

namespace {
constexpr int W_SURGE[4]   = { +1, +1, +1, +1 };
constexpr int W_LATERAL[4] = { +1, -1, +1, -1 };
constexpr int W_YAW[4]     = { -1, +1, +1, -1 };

int toPulse(float v, int neutralUs, int minUs, int maxUs) {
    return std::clamp(neutralUs + static_cast<int>(v * DELTA_US), minUs, maxUs);
}
} // namespace

std::array<int, 8> compute(float surge, float lateral, float yaw, float vertical,
                           float rollCorr, float pitchCorr, const std::array<int, 8> &neutralUs,
                           int minUs, int maxUs) {
    std::array<int, 8> m{};
    for (int i = 0; i < 4; ++i) {
        m[i] = toPulse(surge * W_SURGE[i] + lateral * W_LATERAL[i] + yaw * W_YAW[i], neutralUs[i], minUs, maxUs);
    }
    // M5=on sag, M6=on sol, M7=arka sag, M8=arka sol (bkz. MotorDiagramWidget).
    // Sabitleme kapaliyken rollCorr/pitchCorr 0 gelir, dördü de ayni deger olur.
    // Pitch negatif (burun asagida) oldugunda on motorlarin ARTMASI, arka
    // motorlarin AZALMASI gerekir (burnu yukari kaldirip duzeltmek icin) -
    // bu yuzden pitchCorr on'a eksi, arkaya arti isaretle ekleniyor.
    const float verticalV[4] = {
        vertical + rollCorr - pitchCorr, // M5 on-sag
        vertical - rollCorr - pitchCorr, // M6 on-sol
        vertical + rollCorr + pitchCorr, // M7 arka-sag
        vertical - rollCorr + pitchCorr, // M8 arka-sol
    };
    for (int i = 0; i < 4; ++i) {
        m[4 + i] = toPulse(verticalV[i], neutralUs[4 + i], minUs, maxUs);
    }
    return m;
}

} // namespace MotorMixer
