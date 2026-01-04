#include <HardwareSerial.h>

// SESUAIKAN PIN C3 LU DISINI
#define TX_PIN 21 
#define RX_PIN 20 
#define K_LINE_BAUD 10400

// ESP32-C3 menggunakan Serial1 untuk UART tambahan
HardwareSerial bike(1); 

// ==================== UTILITY FUNCTIONS ====================

uint8_t calcChecksum(uint8_t* data, int len) {
  uint16_t sum = 0;
  for(int i = 0; i < len; i++) sum += data[i];
  return (uint8_t)(0x100 - (sum & 0xFF));
}

void printHex(uint8_t* data, int len) {
  for(int i = 0; i < len; i++) {
    if(data[i] < 0x10) Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

// ==================== K-LINE CORE ====================

void setupKLineUART() {
  // BERSIH: Tanpa manipulasi register yang bikin brick
  bike.begin(K_LINE_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(50);
  Serial.println("[UART] K-Line Active @ 10400");
}

void initPulse() {
  Serial.println("[PULSE] Starting 70/130ms Method...");
  
  bike.end(); 
  pinMode(TX_PIN, OUTPUT);
  
  // Logika Hardware: GPIO LOW = K-Line LOW
  // Pulse 1
  digitalWrite(TX_PIN, LOW);   delay(70);  
  digitalWrite(TX_PIN, HIGH);  delay(130); 
  
  delay(50);
  
  // Pulse 2
  digitalWrite(TX_PIN, LOW);   delay(70);  
  digitalWrite(TX_PIN, HIGH);  delay(130); 
  
  digitalWrite(TX_PIN, HIGH); 
  Serial.println("[PULSE] Done");
}

bool sendRequest(uint8_t* cmd, int len, uint8_t* resBuf, int &resLen) {
  bike.write(cmd, len);
  bike.flush();
  
  // Buang Echo
  int echoCount = 0;
  unsigned long echoTimeout = millis();
  while ((millis() - echoTimeout < 150) && (echoCount < len)) {
    if (bike.available()) {
      bike.read();
      echoCount++;
    }
  }

  // Baca Respon ECU
  resLen = 0;
  unsigned long readTimeout = millis();
  while ((millis() - readTimeout < 500) && (resLen < 128)) {
    if (bike.available()) {
      resBuf[resLen++] = bike.read();
    }
  }
  return (resLen > 0);
}

// ==================== MAIN PROCESS ====================

void detectTable() {
  Serial.println("\n--- Detecting Table 11 ---");
  uint8_t cmd[] = {0x72, 0x05, 0x71, 0x11, 0x00};
  cmd[4] = calcChecksum(cmd, 4);
  
  uint8_t rxBuf[128];
  int rxLen = 0;

  if (sendRequest(cmd, sizeof(cmd), rxBuf, rxLen)) {
    Serial.print("[RX] Data: ");
    printHex(rxBuf, rxLen);
  } else {
    Serial.println("[!] No Data from Table 11");
  }
}

void bangunkanECU() {
  initPulse();
  setupKLineUART();
  
  uint8_t wakeUp[] = {0xFE, 0x04, 0x72, 0x8C}; 
  uint8_t rxBuf[32];
  int rxLen = 0;

  Serial.println("[TX] Sending 0xFE Wake-up...");
  if (sendRequest(wakeUp, sizeof(wakeUp), rxBuf, rxLen)) {
    Serial.print("[RX] ECU: ");
    printHex(rxBuf, rxLen);
    
    if (rxLen >= 2) {
      Serial.println(">>> CONNECTED");
      delay(500);
      detectTable();
    }
  } else {
    Serial.println(">>> NO RESPONSE");
  }
}

void setup() {
  Serial.begin(115200); // Jalur USB Monitor
  delay(3000);
  Serial.println("\n--- C3 K-LINE SYSTEM START ---");

  bangunkanECU();
}

void loop() {
  // Idle
}
