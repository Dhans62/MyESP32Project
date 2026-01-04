#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <HardwareSerial.h>

#define TX_PIN 21 
#define RX_PIN 20 
#define K_LINE_BAUD 10400

HardwareSerial bike(1);

// UUID untuk BLE (Gue samain ama kode lama lu biar gampang connect)
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;

// ==================== BLE LOGGING ====================
void logToBLE(String msg) {
  // Tetap cetak ke Serial (buat jaga-jaga kalau suatu saat dicolok ke PC)
  Serial.println(msg); 
  
  if (deviceConnected) {
    // Kirim per 20 karakter karena limit MTU BLE standar
    int maxLen = 20;
    for(int i = 0; i < msg.length(); i += maxLen) {
      String chunk = msg.substring(i, min(i + maxLen, (int)msg.length()));
      pTxCharacteristic->setValue(chunk.c_str());
      pTxCharacteristic->notify();
      delay(20); 
    }
    pTxCharacteristic->setValue("\n");
    pTxCharacteristic->notify();
  }
}

// ==================== UTILITY ====================
uint8_t calcChecksum(uint8_t* data, int len) {
  uint16_t sum = 0;
  for(int i = 0; i < len; i++) sum += data[i];
  return (uint8_t)(0x100 - (sum & 0xFF));
}

String hexToString(uint8_t* data, int len) {
  String res = "";
  for(int i = 0; i < len; i++) {
    if(data[i] < 0x10) res += "0";
    res += String(data[i], HEX) + " ";
  }
  return res;
}

// ==================== K-LINE CORE ====================
void setupKLineUART() {
  bike.begin(K_LINE_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(50);
  logToBLE("[UART] 10400 OK");
}

void initPulse() {
  logToBLE("[INIT] Pulse 70/130ms");
  bike.end();
  pinMode(TX_PIN, OUTPUT);
  
  // Pulse 1
  digitalWrite(TX_PIN, LOW);   delay(70);
  digitalWrite(TX_PIN, HIGH);  delay(130);
  delay(50);
  // Pulse 2
  digitalWrite(TX_PIN, LOW);   delay(70);
  digitalWrite(TX_PIN, HIGH);  delay(130);
  
  digitalWrite(TX_PIN, HIGH); 
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

// ==================== MAIN PROCESS ====================
void bangunkanECU() {
  initPulse();
  
  uint8_t wakeUp[] = {0xFE, 0x04, 0x72, 0x8C};
  uint8_t rxBuf[32];
  int rxLen = 0;

  logToBLE("[TX] Wake-up 0xFE");
  if (sendRequest(wakeUp, sizeof(wakeUp), rxBuf, rxLen)) {
    logToBLE("[RX] " + hexToString(rxBuf, rxLen));
    if (rxLen >= 2) logToBLE(">>> CONNECTED");
  } else {
    logToBLE(">>> NO RESP");
  }
}

// ==================== BLE CALLBACKS ====================
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      pServer->getAdvertising()->start();
    }
};

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi BLE
  BLEDevice::init("C3_KLINE_LOG");
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

  // Tunggu lu konek ke BLE HP sebelum mulai
  // Biar log awalnya gak kelewat
  while (!deviceConnected) {
    delay(500); 
    Serial.println("Waiting for BLE connection...");
  }
  
  delay(1000);
  logToBLE("=== C3 BLE SCANNER READY ===");
  bangunkanECU();
}

void loop() {
  // Nanti tambahin polling data di sini kalau udah konek
}
