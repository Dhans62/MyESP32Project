#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <HardwareSerial.h>

#define TX_PIN 21 
#define RX_PIN 20 
#define LED_PIN 8   // LED bawaan C3 (GPIO 8). Sesuaikan kalau beda.
#define K_LINE_BAUD 10400

HardwareSerial bike(1);

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;

void logToBLE(String msg) {
  if (deviceConnected) {
    pTxCharacteristic->setValue(msg.c_str());
    pTxCharacteristic->notify();
    delay(50);
  }
  Serial.println(msg);
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { 
      deviceConnected = true; 
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      pServer->getAdvertising()->start();
    }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  
  // Inisialisasi BLE
  BLEDevice::init("TES_C3_KLINE");
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

  // Setup UART K-Line
  bike.begin(K_LINE_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  if (!deviceConnected) {
    // Skenario 1: BLE Belum Connect -> LED Kedip Cepat
    digitalWrite(LED_PIN, HIGH); delay(100);
    digitalWrite(LED_PIN, LOW);  delay(100);
  } else {
    // Skenario 2: BLE Connect -> LED Stay ON
    digitalWrite(LED_PIN, HIGH);
    
    logToBLE("--- TEST JALUR K-LINE ---");
    
    // Kirim satu byte dummy (0xFE) ke K-Line
    uint8_t testByte = 0xFE;
    bike.write(testByte);
    bike.flush();
    logToBLE("TX Sent: 0xFE");

    // Cek apakah ada Echo (Data balik dari rangkaian lu sendiri)
    delay(100);
    if (bike.available()) {
      uint8_t echo = bike.read();
      logToBLE("RX Echo: 0x" + String(echo, HEX));
      
      if (echo == testByte) {
        logToBLE("STATUS: HARDWARE LURUS (ECHO OK)");
      } else {
        logToBLE("STATUS: DATA KORUP / SALAH LOGIKA");
      }
    } else {
      logToBLE("STATUS: NO ECHO (Jalur Putus/6N137 Mati)");
    }

    delay(3000); // Ulang tiap 3 detik
  }
}
