#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <HardwareSerial.h>

// --- PIN KHUSUS ESP32-C3 ---
#define RX_PIN 20
#define TX_PIN 21
HardwareSerial bike(1); // C3 gunakan UART1

#define SERVICE_UUID           "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;

int rpm = 0, ectc = 0, tpsp = 0;
float batv = 0.0;
unsigned long prevMs = 0;

void logBLE(String s) {
  if (deviceConnected) {
    pTxCharacteristic->setValue(s.c_str());
    pTxCharacteristic->notify();
  }
  Serial.println(s);
}

// --- INISIALISASI K-LINE VERSI C3 ---
void klineInit() {
  // Inisialisasi dengan parameter INVERT = TRUE untuk 6N137
  // Ini menggantikan WRITE_PERI_REG yang lama
  bike.begin(10400, SERIAL_8N1, RX_PIN, TX_PIN, true); 
  logBLE("[SYS] K-Line Ready (C3 Inverted)");
}

void wakeup() {
  logBLE("[SYS] Wakeup...");
  byte w[] = {0xFE, 0x04, 0x72, 0x8C};
  bike.write(w, 4); delay(200);
  byte i[] = {0x72, 0x05, 0x00, 0xF0, 0x99};
  bike.write(i, 5); delay(300);
  while(bike.available()) bike.read(); 
}

void readTable11() {
  byte cmd[] = {0x72, 0x05, 0x71, 0x11, 0x07};
  while(bike.available()) bike.read();
  bike.write(cmd, 5);
  delay(180);

  uint8_t buf[64];
  int len = 0;
  while(bike.available() && len < 64) buf[len++] = bike.read();

  if (len < 15) return;

  int h = -1;
  for(int i=0; i < len - 10; i++) {
    if(buf[i] == 0x02 && buf[i+2] == 0x71) { 
      h = i; break;
    }
  }

  if (h != -1) {
    rpm = (uint16_t)(buf[h + 9] << 8) | buf[h + 10];
    tpsp = buf[h + 12] / 1.6;        
    ectc = buf[h + 14] - 40;        
    batv = buf[h + 21] / 10.0;      

    String out = "RPM:" + String(rpm) + " ECT:" + String(ectc) + "C AKI:" + String(batv, 1) + "V";
    logBLE(out);
  } else {
    logBLE("SYNC ERR: No Header");
  }
}

class MyCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* p) { deviceConnected = true; }
    void onDisconnect(BLEServer* p) { deviceConnected = false; p->getAdvertising()->start(); }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("HONDA_C3_PRO"); // Ganti nama agar Anda tahu ini versi C3
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyCallbacks());
  BLEService *pSvc = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pSvc->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());
  pSvc->start();
  pServer->getAdvertising()->start();
  klineInit();
}

void loop() {
  if (deviceConnected && millis() - prevMs > 400) {
    readTable11();
    prevMs = millis();
  }
}
