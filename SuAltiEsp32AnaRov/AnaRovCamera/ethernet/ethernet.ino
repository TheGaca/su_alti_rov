#include <Arduino.h>
#include <ETH.h>
#include <SPI.h>

#define ETH_PHY_TYPE ETH_PHY_W5500
#define ETH_PHY_ADDR 1

#define ETH_CS   15
#define ETH_IRQ  -1
#define ETH_RST  -1

#define ETH_SCK  14
#define ETH_MISO 12
#define ETH_MOSI 13

bool ethernetConnected = false;

void onNetworkEvent(arduino_event_id_t event,
                    arduino_event_info_t info)
{
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("Ethernet baslatildi");
      ETH.setHostname("esp32-camera");
      break;

    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("Ethernet kablosu baglandi");
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.println("Ethernet IP hazir");

      Serial.print("IP: ");
      Serial.println(ETH.localIP());

      Serial.print("Subnet: ");
      Serial.println(ETH.subnetMask());

      Serial.print("MAC: ");
      Serial.println(ETH.macAddress());

      ethernetConnected = true;
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("Ethernet kablosu ayrildi");
      ethernetConnected = false;
      break;

    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("Ethernet IP kayboldu");
      ethernetConnected = false;
      break;

    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("Ethernet durduruldu");
      ethernetConnected = false;
      break;

    default:
      break;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1500);

  Network.onEvent(onNetworkEvent);

  pinMode(ETH_CS, OUTPUT);
  digitalWrite(ETH_CS, HIGH);

  SPI.begin(
    ETH_SCK,
    ETH_MISO,
    ETH_MOSI,
    ETH_CS
  );

  Serial.println("W5500 baslatiliyor...");

  if (!ETH.begin(
        ETH_PHY_TYPE,
        ETH_PHY_ADDR,
        ETH_CS,
        ETH_IRQ,
        ETH_RST,
        SPI)) {
    Serial.println("HATA: W5500 baslatilamadi");
    return;
  }

  IPAddress ip(192, 168, 3, 201);
  IPAddress gateway(0, 0, 0, 0);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(0, 0, 0, 0);

  if (!ETH.config(ip, gateway, subnet, dns)) {
    Serial.println("HATA: Sabit IP ayarlanamadi");
    return;
  }

  Serial.println("Sabit IP ayarlandi");
}

void loop()
{
  if (ethernetConnected) {
    Serial.print("Ethernet aktif: ");
    Serial.println(ETH.localIP());
  } else {
    Serial.println("Ethernet baglantisi bekleniyor...");
  }

  delay(3000);
}
