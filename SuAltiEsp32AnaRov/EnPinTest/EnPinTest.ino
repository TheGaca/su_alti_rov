// EN pini / acilis guvenilirligi test kodu.
// ESP32'yi PCB'den cikarip TEK BASINA (kamera, W5500, baska hicbir modul
// BAGLI DEGILKEN) test etmek icindir - amac, cip her guc verisinde kendi
// basina, hicbir ek donanim/karmasik kod olmadan duzgun acilip acilmadigini
// gormek. Eger bu test de ayni "sessiz kaliyor, RST'ye kadar hicbir sey
// yazmiyor" davranisini gosterirse, sorun kesinlikle EN pini/guc/modulun
// kendisinde demektir - PCB'deki baska bir sey (W5500, kamera vb.) suclu
// degildir.
//
// Kullanim: bu sketch'i yukleyin, Serial Monitor'u acik birakin (115200
// baud), sonra USB'yi COPARIP TAKMADAN sadece kartin kendi VIN/5V veya 3V3
// besleme hattini birkac kez kesip verin (gercek power-on-reset testi icin).
// Her guc verisinde "=== EN/BOOT TEST basladi ===" satirinin hemen cikip
// cikmadigina bakin.

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("=== EN/BOOT TEST basladi ===");

  esp_reset_reason_t reason = esp_reset_reason();
  Serial.print("Reset sebebi: ");
  switch (reason) {
    case ESP_RST_POWERON:  Serial.println("POWERON (gercek guc acilisi ya da EN pini toggle)"); break;
    case ESP_RST_EXT:      Serial.println("EXT (harici reset pini)"); break;
    case ESP_RST_SW:       Serial.println("SW (yazilimsal reset - esp_restart())"); break;
    case ESP_RST_PANIC:    Serial.println("PANIC (kod cokmesi)"); break;
    case ESP_RST_INT_WDT:  Serial.println("INT_WDT (ic watchdog)"); break;
    case ESP_RST_TASK_WDT: Serial.println("TASK_WDT (gorev watchdog'u)"); break;
    case ESP_RST_WDT:      Serial.println("WDT (diger watchdog)"); break;
    case ESP_RST_BROWNOUT: Serial.println("BROWNOUT - GERILIM DUSUSU! Guc kaynagi yetersiz/kararsiz."); break;
    case ESP_RST_SDIO:     Serial.println("SDIO"); break;
    default:               Serial.println((int)reason); break;
  }

  pinMode(2, OUTPUT); // cogu ESP32 devkit'inde onboard LED (GPIO2) - yoksa zararsiz, bos pin
}

unsigned long lastPrint = 0;
bool ledState = false;

void loop() {
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    Serial.printf("ALIVE t=%lus\n", millis() / 1000);
    ledState = !ledState;
    digitalWrite(2, ledState);
  }
}
