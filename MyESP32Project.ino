// =====================================================
// Honda K-Line Scanner - Verified Protocol Implementation
// Based on proven YouTube reference, adapted for PC817 hardware
// Hardware: ESP32, PC817 TX, BC547 RX
// =====================================================

#include <HardwareSerial.h>

#define K_LINE_RX 16
#define K_LINE_TX 17
#define K_LINE_BAUD 10400
#define DEBUG_BAUD 115200

#define PACKET_BUFFER_SIZE 128

HardwareSerial bike(2);

// ECU Communication Protocol
byte ECU_WAKEUP_MESSAGE[] = {0xFE, 0x04, 0x72, 0x8C};
byte ECU_INIT_MESSAGE[] = {0x72, 0x05, 0x00, 0xF0, 0x99};

// ECU State
bool ecuConnected = false;
byte ecuTableType = 0;  // 11, 17, 10, atau 0 jika tidak terdeteksi
byte displayMode = 0;   // Untuk tracking mode display

// Live Data Variables
int rpm = 0;
float tpsv = 0.0;
int tpsp = 0;
float batv = 0.0;
float ectv = 0.0;
int ectc = 0;
float eotv = 0.0;
int eotc = 0;
float injj = 0.0;

// Timing
unsigned long lastPollTime = 0;
const unsigned long POLL_INTERVAL = 100;  // 10Hz

void setup() {
  Serial.begin(DEBUG_BAUD);
  delay(2000);
  
  Serial.println("\n========================================");
  Serial.println("  Honda K-Line Diagnostic Scanner");
  Serial.println("  Hardware: PC817 + BC547");
  Serial.println("  Protocol: ISO-9141 (Honda Modified)");
  Serial.println("========================================\n");
  
  // Initialize K-Line with hardware inversion for BC547
  initKLineUART();
  
  // Execute wake-up sequence (CRITICAL: Double wake-up per Honda requirement)
  Serial.println("[INIT] Starting Honda double wake-up sequence...");
  if (!initComms()) {
    Serial.println("[ERROR] Failed to initialize K-Line timing");
    return;
  }
  
  // Perform ECU handshake
  Serial.println("[INIT] Sending wake-up and init messages...");
  if (!performECUHandshake()) {
    Serial.println("[ERROR] ECU handshake failed");
    Serial.println("[INFO] Check: 1) K-Line connection, 2) Ignition ON, 3) Ground connection");
    return;
  }
  
  // Detect ECU table type
  Serial.println("\n[DETECT] Probing ECU table types...");
  detectECUTableType();
  
  if (ecuTableType == 0) {
    Serial.println("[ERROR] Could not identify ECU table type");
    Serial.println("[INFO] ECU may not be compatible or communication is unstable");
    return;
  }
  
  Serial.printf("[SUCCESS] ECU detected with Table %d support\n", ecuTableType);
  Serial.println("\n[READY] Starting live data polling...\n");
  ecuConnected = true;
}

void loop() {
  if (!ecuConnected) {
    Serial.println("[ERROR] ECU not connected. System halted.");
    delay(5000);
    return;
  }
  
  // Poll ECU data based on detected table type
  unsigned long currentTime = millis();
  if (currentTime - lastPollTime >= POLL_INTERVAL) {
    lastPollTime = currentTime;
    
    switch(ecuTableType) {
      case 11:
        pollTable11();
        break;
      case 17:
        pollTable17();
        break;
      case 10:
        pollTable10();
        break;
      default:
        Serial.println("[ERROR] Invalid table type");
        ecuConnected = false;
        return;
    }
    
    displayLiveData();
  }
  
  delay(10);  // Keep loop responsive
}

void initKLineUART() {
  bike.begin(K_LINE_BAUD, SERIAL_8N1, K_LINE_RX, K_LINE_TX);
  
  // Enable RX inversion for BC547 compensation
  uint32_t conf0_addr = 0x3FF6E020;  // UART2_CONF0_REG
  uint32_t conf0_val = READ_PERI_REG(conf0_addr);
  conf0_val |= (1 << 19);  // Set RXD_INV bit
  WRITE_PERI_REG(conf0_addr, conf0_val);
  
  Serial.println("[UART] Serial2 initialized at 10400 baud with RX inversion");
  
  // Flush any residual data
  while (bike.available()) {
    bike.read();
  }
}

bool initComms() {
  // CRITICAL: Honda requires DOUBLE wake-up sequence
  // This is non-standard but proven necessary for certain Honda ECUs
  
  // First wake-up pulse
  pinMode(K_LINE_TX, OUTPUT);
  digitalWrite(K_LINE_TX, LOW);
  delay(70);
  digitalWrite(K_LINE_TX, HIGH);
  delay(130);
  
  // Inter-pulse delay
  delay(50);
  
  // Second wake-up pulse
  digitalWrite(K_LINE_TX, LOW);
  delay(70);
  digitalWrite(K_LINE_TX, HIGH);
  delay(130);
  
  Serial.println("[INIT] Double wake-up pulse completed (70ms+130ms, pause 50ms, 70ms+130ms)");
  
  // Re-initialize UART after bitbang
  initKLineUART();
  delay(50);
  
  return true;
}

bool performECUHandshake() {
  // Send wake-up message
  bike.write(ECU_WAKEUP_MESSAGE, sizeof(ECU_WAKEUP_MESSAGE));
  bike.flush();
  delay(200);  // Wait for ECU to process
  
  // Send init message
  bike.write(ECU_INIT_MESSAGE, sizeof(ECU_INIT_MESSAGE));
  bike.flush();
  delay(50);
  
  // Read response (includes echo + actual response)
  int buffCount = 0;
  byte buff[32];
  unsigned long startTime = millis();
  
  while ((millis() - startTime < 500) && (buffCount < 32)) {
    if (bike.available()) {
      buff[buffCount++] = bike.read();
    }
  }
  
  Serial.printf("[HANDSHAKE] Received %d bytes: ", buffCount);
  for (int i = 0; i < buffCount; i++) {
    Serial.printf("%02X ", buff[i]);
  }
  Serial.println();
  
  // Calculate checksum of response (validation per reference code)
  int checksum = 0;
  for (int i = 0; i < buffCount; i++) {
    checksum += buff[i];
  }
  
  // Reference code uses checksum == 0x600 as success indicator
  // This is empirical validation, not standard protocol
  if (checksum == 0x600 || buffCount >= 10) {
    Serial.println("[HANDSHAKE] ECU responded successfully");
    return true;
  }
  
  Serial.printf("[HANDSHAKE] Unexpected checksum: 0x%X (expected 0x600)\n", checksum);
  return false;
}

void detectECUTableType() {
  // Try Table 11
  Serial.print("[DETECT] Probing Table 11... ");
  byte cmd11[] = {0x72, 0x05, 0x71, 0x11, 0x00};
  cmd11[4] = calcChecksum(cmd11, 4);
  
  bike.write(cmd11, sizeof(cmd11));
  bike.flush();
  delay(50);
  
  int count11 = readResponse(200);
  Serial.printf("Response length: %d bytes\n", count11);
  
  if (count11 == 30) {
    ecuTableType = 11;
    displayMode = 1;
    return;
  } else if (count11 == 29) {
    ecuTableType = 11;
    displayMode = 7;  // Sport variant
    return;
  }
  
  // Try Table 17
  Serial.print("[DETECT] Probing Table 17... ");
  byte cmd17[] = {0x72, 0x05, 0x71, 0x17, 0x00};
  cmd17[4] = calcChecksum(cmd17, 4);
  
  bike.write(cmd17, sizeof(cmd17));
  bike.flush();
  delay(50);
  
  int count17 = readResponse(200);
  Serial.printf("Response length: %d bytes\n", count17);
  
  if (count17 == 28) {
    ecuTableType = 17;
    displayMode = 2;
    return;
  } else if (count17 == 29) {
    ecuTableType = 17;
    displayMode = 3;  // ESP variant
    return;
  }
  
  // Try Table 10
  Serial.print("[DETECT] Probing Table 10... ");
  byte cmd10[] = {0x72, 0x05, 0x71, 0x10, 0x00};
  cmd10[4] = calcChecksum(cmd10, 4);
  
  bike.write(cmd10, sizeof(cmd10));
  bike.flush();
  delay(50);
  
  int count10 = readResponse(200);
  Serial.printf("Response length: %d bytes\n", count10);
  
  if (count10 == 27) {
    ecuTableType = 10;
    displayMode = 8;
    return;
  }
  
  // No table detected
  ecuTableType = 0;
}

int readResponse(unsigned long timeoutMs) {
  byte buff[PACKET_BUFFER_SIZE];
  int buffCount = 0;
  unsigned long startTime = millis();
  
  while ((millis() - startTime < timeoutMs) && (buffCount < PACKET_BUFFER_SIZE)) {
    if (bike.available()) {
      buff[buffCount++] = bike.read();
    }
  }
  
  // Flush any remaining data
  while (bike.available()) {
    bike.read();
  }
  
  return buffCount;
}

uint8_t calcChecksum(const uint8_t* data, uint8_t len) {
  // Standard Honda checksum: 0x100 - (sum & 0xFF)
  uint16_t sum = 0;
  for (uint8_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return (uint8_t)(0x100 - (sum & 0xFF));
}

void pollTable11() {
  byte cmd[] = {0x72, 0x07, 0x72, 0x11, 0x00, 0x10, 0x00};
  cmd[6] = calcChecksum(cmd, 6);
  
  bike.write(cmd, sizeof(cmd));
  bike.flush();
  delay(50);
  
  byte buff[PACKET_BUFFER_SIZE];
  int buffCount = 0;
  unsigned long startTime = millis();
  
  while ((millis() - startTime < 150) && (buffCount < PACKET_BUFFER_SIZE)) {
    if (bike.available()) {
      buff[buffCount++] = bike.read();
    }
  }
  
  if (buffCount < 20) {
    Serial.println("[POLL] Insufficient data from Table 11");
    return;
  }
  
  // Parse data (skip echo bytes 0-6, data starts at index 7)
  int dataStart = 7;
  
  // RPM: bytes 11-12 (word, big endian)
  rpm = (buff[dataStart + 4] << 8) | buff[dataStart + 5];
  
  // Battery Voltage: byte 13
  batv = calcValueDivide256(buff[dataStart + 6]);
  
  // TPS: byte 14
  tpsp = (buff[dataStart + 7] * 100) / 255;
  tpsv = calcValueDivide256(buff[dataStart + 7]);
  
  // ECT: byte 15
  ectc = buff[dataStart + 8] - 40;
  ectv = calcValueDivide256(buff[dataStart + 8]);
  
  // Injector duration: bytes 18-19
  uint16_t injRaw = (buff[dataStart + 11] << 8) | buff[dataStart + 12];
  injj = injRaw / (65535.0 * 265.5);
}

void pollTable17() {
  byte cmd[] = {0x72, 0x07, 0x72, 0x17, 0x00, 0x10, 0x00};
  cmd[6] = calcChecksum(cmd, 6);
  
  bike.write(cmd, sizeof(cmd));
  bike.flush();
  delay(50);
  
  byte buff[PACKET_BUFFER_SIZE];
  int buffCount = 0;
  unsigned long startTime = millis();
  
  while ((millis() - startTime < 150) && (buffCount < PACKET_BUFFER_SIZE)) {
    if (bike.available()) {
      buff[buffCount++] = bike.read();
    }
  }
  
  if (buffCount < 20) {
    Serial.println("[POLL] Insufficient data from Table 17");
    return;
  }
  
  // Similar parsing logic for Table 17
  // Structure may differ slightly from Table 11
  int dataStart = 7;
  
  rpm = (buff[dataStart + 4] << 8) | buff[dataStart + 5];
  batv = calcValueDivide256(buff[dataStart + 6]);
  tpsp = (buff[dataStart + 7] * 100) / 255;
}

void pollTable10() {
  byte cmd[] = {0x72, 0x07, 0x72, 0x10, 0x00, 0x10, 0x00};
  cmd[6] = calcChecksum(cmd, 6);
  
  bike.write(cmd, sizeof(cmd));
  bike.flush();
  delay(50);
  
  byte buff[PACKET_BUFFER_SIZE];
  int buffCount = 0;
  unsigned long startTime = millis();
  
  while ((millis() - startTime < 150) && (buffCount < PACKET_BUFFER_SIZE)) {
    if (bike.available()) {
      buff[buffCount++] = bike.read();
    }
  }
  
  if (buffCount < 20) {
    Serial.println("[POLL] Insufficient data from Table 10");
    return;
  }
  
  int dataStart = 7;
  rpm = (buff[dataStart + 4] << 8) | buff[dataStart + 5];
  batv = calcValueDivide256(buff[dataStart + 6]);
}

void displayLiveData() {
  Serial.printf("[DATA] RPM: %d | Battery: %.2fV | TPS: %d%% (%.2fV) | ECT: %d°C (%.2fV) | Inj: %.3fms\n",
                rpm, batv, tpsp, tpsv, ectc, ectv, injj);
}

float calcValueDivide256(float val) {
  return (val * 5.0) / 256.0;
}
