// MotorMixer birim testleri - Qt'ye ihtiyac duymaz, "ctest" ile calisir.
// Amac: motor karisimindaki isaret/siniri bozan bir degisikligin havuza
// gitmeden once masada yakalanmasi.
#include "MotorMixer.hpp"

#include <cstdio>
#include <cstdlib>

static int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::fprintf(stderr, "BASARISIZ %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                      \
        }                                                                    \
    } while (0)

using namespace MotorMixer;

int main() {
    // Notr girdi -> tum motorlar notr darbede
    {
        auto m = compute(0, 0, 0, 0);
        for (int i = 0; i < 8; ++i) CHECK(m[i] == NEUTRAL_US);
    }

    // Tam ileri surge: 4 yatay motor ayni yonde artar, dikeyler notr kalir
    {
        auto m = compute(1, 0, 0, 0);
        for (int i = 0; i < 4; ++i) CHECK(m[i] == MAX_US); // 1490+200=1690 -> 1600'e kirpilir
        for (int i = 4; i < 8; ++i) CHECK(m[i] == NEUTRAL_US);
    }

    // Tam geri surge: yatay motorlar taban sinirina kirpilir (1490-200=1290 < 1295)
    {
        auto m = compute(-1, 0, 0, 0);
        for (int i = 0; i < 4; ++i) CHECK(m[i] == MIN_US);
    }

    // Asiri girdi bile sinirlarin disina cikamaz
    {
        auto m = compute(10, 10, 10, 10);
        for (int i = 0; i < 8; ++i) CHECK(m[i] >= MIN_US && m[i] <= MAX_US);
        auto n = compute(-10, -10, -10, -10);
        for (int i = 0; i < 8; ++i) CHECK(n[i] >= MIN_US && n[i] <= MAX_US);
    }

    // Yaw isaretleri: saga donus (+) icin W_YAW = {-1,+1,+1,-1}
    {
        auto m = compute(0, 0, 0.5f, 0);
        CHECK(m[0] < NEUTRAL_US);
        CHECK(m[1] > NEUTRAL_US);
        CHECK(m[2] > NEUTRAL_US);
        CHECK(m[3] < NEUTRAL_US);
    }

    // Lateral isaretleri: saga kayma (+) icin W_LATERAL = {+1,-1,+1,-1}
    {
        auto m = compute(0, 0.5f, 0, 0);
        CHECK(m[0] > NEUTRAL_US);
        CHECK(m[1] < NEUTRAL_US);
        CHECK(m[2] > NEUTRAL_US);
        CHECK(m[3] < NEUTRAL_US);
    }

    // Dikey + roll duzeltmesi: rollCorr sag motorlara (M5, M7) arti,
    // sol motorlara (M6, M8) eksi eklenir
    {
        auto m = compute(0, 0, 0, 0, 0.25f, 0.0f);
        CHECK(m[4] > NEUTRAL_US); // M5 on-sag
        CHECK(m[5] < NEUTRAL_US); // M6 on-sol
        CHECK(m[6] > NEUTRAL_US); // M7 arka-sag
        CHECK(m[7] < NEUTRAL_US); // M8 arka-sol
        // Yatay motorlar duzeltmeden etkilenmez
        for (int i = 0; i < 4; ++i) CHECK(m[i] == NEUTRAL_US);
    }

    // Pitch duzeltmesi: pozitif pitchCorr on motorlari (M5, M6) azaltir,
    // arka motorlari (M7, M8) artirir
    {
        auto m = compute(0, 0, 0, 0, 0.0f, 0.25f);
        CHECK(m[4] < NEUTRAL_US); // M5 on-sag
        CHECK(m[5] < NEUTRAL_US); // M6 on-sol
        CHECK(m[6] > NEUTRAL_US); // M7 arka-sag
        CHECK(m[7] > NEUTRAL_US); // M8 arka-sol
    }

    // Simetri: ayni buyuklukte zit girdiler notre gore ayna goruntusu olmali
    // (kirpilmaya ugramayan kucuk girdiyle)
    {
        auto a = compute(0.3f, 0, 0, 0);
        auto b = compute(-0.3f, 0, 0, 0);
        for (int i = 0; i < 4; ++i) CHECK(a[i] - NEUTRAL_US == NEUTRAL_US - b[i]);
    }

    if (failures == 0) {
        std::printf("Tum MotorMixer testleri gecti.\n");
        return EXIT_SUCCESS;
    }
    std::fprintf(stderr, "%d test basarisiz!\n", failures);
    return EXIT_FAILURE;
}
