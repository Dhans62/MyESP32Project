#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <HardwareSerial.h>

// PINOUT ESP32-C3
#define TX_PIN 21 
#define RX_PIN 20 
#define LED_PIN 8   // Biasanya LED bawaan C3 di GPIO 8, sesuaikan kalau beda
#define K_LINE_BAUD 10400

HardwareSerial bike(1);

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;

// ==================== INDIKATOR & LOGGING ====================
void logToBLE(String msg) {
  Serial.println(msg); 
  if (deviceConnected) {
    int maxLen = 20;
    for(int i = 0; i < msg.length(); i += maxLen) {
      String chunk = msg.substring(i, min(i + maxLen, (int)msg.length()));
      pTxCharacteristic->setValue(chunk.c_str());
      pTxCharacteristic->notify();
      delay(25); 
    }
  }
}

// ==================== K-LINE CORE ====================
void setupKLineUART() {
  bike.begin(K_LINE_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(50);
}

void initPulse() {
  logToBLE("[INIT] Pulse...");
  bike.end();
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);   delay(70);
  digitalWrite(TX_PIN, HIGH);  delay(130);
  delay(50);
  digitalWrite(TX_PIN, LOW);   delay(70);
  digitalWrite(TX_PIN, HIGH);  delay(130);
  setupKLineUART();
}

bool sendRequest(uint8_t* cmd, int len, uint8_t* resBuf, int &resLen) {
  bike.write(cmd, len);
  bike.flush();
  
  // Buang Echo
  int echoCount = 0;
  unsigned long echoT = millis();
  while ((millis() - echoT < 150) && (echoCount < len)) {
    if (bike.available()) { bike.read(); echoCount++; }
  }

  // Baca Respon
  resLen = 0;
  unsigned long readT = millis();
  while ((millis() - readT < 500) && (resLen < 64)) {
    if (bike.available()) { resBuf[resLen++] = bike.read(); }
  }
  return (resLen > 0);
}

// ==================== BLE CALLBACKS ====================
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { 
      deviceConnected = true; 
      digitalWrite(LED_PIN, HIGH); // Stay ON saat BLE connect
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      pServer->getAdvertising()->start();
    }
};

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  
  // Inisialisasi BLE
  BLEDevice::init("C3_KLINE_V2");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  pServer->getAdvertising()->start();
}

void loop() {
  // Jika BLE belum connect, LED kedip cepat
  if (!deviceConnected) {
    digitalWrite(LED_PIN, HIGH); delay(100);
    digitalWrite(LED_PIN, LOW);  delay(100);
  } else {
    // Jika sudah connect, coba bangunkan ECU
    logToBLE("=== START WAKEUP ===");
    initPulse();
    
    uint8_t wakeUp[] = {0xFE, 0x04, 0x72, 0x8C};
    uint8_t rxBuf[32];
    int rxLen = 0;

    if (sendRequest(wakeUp, sizeof(wakeUp), rxBuf, rxLen)) {
      logToBLE("ECU RESPONDED!");
      // Kedip lambat tanda sukses komunikasi
      for(int i=0; i<5; i++) {
        digitalWrite(LED_PIN, LOW); delay(500);
        digitalWrite(LED_PIN, HIGH); delay(500);
      }
    } else {
      logToBLE("ECU NO RESP");
    }
    delay(5000); // Tunggu 5 detik sebelum coba lagi
  }
}
