/*
 * Pil gerilim bolucu bench-test sketch'i (Ana ROV'a entegre DEGIL, tek basina).
 * Bolucu lehimlendikten sonra GPIO34 baglantisini dogrulamak icin kullanilir:
 * USB Serial Monitor'da (115200) hem pindeki voltaj hem hesaplanan pil
 * voltaji basilir - multimetreyle kiyasla. Ana koddaki karsiligi:
 * AnaRovBeyin.ino "Pil (12V) Voltaj Olcumu" bolumu (ayni bolucu, ayni oran).
 *
 * Baglanti:  Pil(+) --[100k]-- GPIO34 --[33k]-- GND
 */
const int batteryPin = 34;

// Gerilim bölücü
const float R1 = 100000.0; // 100k
const float R2 = 33000.0;  // 33k

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetPinAttenuation(batteryPin, ADC_11db);
}

void loop() {

  long toplam = 0;

  // 20 ölçümün ortalamasını al
  for (int i = 0; i < 20; i++) {
    toplam += analogReadMilliVolts(batteryPin);
    delay(5);
  }

  float adcMilliVolts = toplam / 20.0;
  float adcVolts = adcMilliVolts / 1000.0;

  float batteryVoltage = adcVolts * ((R1 + R2) / R2);

  Serial.print("ESP32 = ");
  Serial.print(adcVolts, 3);
  Serial.print(" V");

  Serial.print("   Pil = ");
  Serial.print(batteryVoltage, 2);
  Serial.println(" V");

  delay(1000);
}
