#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include "web.h"
#include "driver/gpio.h"
#include "driver/uart.h"

// ==================== HARDWARE ====================
HardwareSerial KLine = Serial2;

const int KLINE_TX_PIN = 17;
const int KLINE_RX_PIN = 16;
const int LED_PIN = 2;

// ==================== WIFI ====================
const char* ap_ssid = "HONDA_KLINE_AP";
const char* ap_pass = "12345678";
IPAddress localIP(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);

WebServer server(80);

// ==================== LOGGING ====================
#define LOG_LINES 200
String logs[LOG_LINES];
int logHead = 0;
int logCount = 0;

void addLog(const String &s) {
  String t = String(millis()) + " | " + s;
  logs[logHead] = t;
  logHead = (logHead + 1) % LOG_LINES;
  if (logCount < LOG_LINES) logCount++;
  Serial.println(t);
}

String getLogs() {
  String out;
  int start = (logHead - logCount + LOG_LINES) % LOG_LINES;
  for (int i = 0; i < logCount; ++i) {
    out += logs[(start + i) % LOG_LINES] + "\n";
  }
  return out;
}

// ==================== ECU STATE ====================
String connectedProtocol = "NONE";
byte ecuMode = 0;
bool ecuConnected = false;

unsigned long lastConnectionAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000;
int connectionFailCount = 0;
const int MAX_FAIL_BEFORE_REBOOT = 10;

// ✅ FIX: Separate watchdogs untuk data dan connection health
unsigned long lastValidData = 0;
unsigned long lastSuccessfulPoll = 0;
const unsigned long DATA_TIMEOUT = 15000;  // 15s tolerance (lebih generous)
const unsigned long POLL_TIMEOUT = 3000;   // 3s per-poll timeout

#define PACKET_BUFFER_SIZE 128

// ==================== ECU DATA ====================
struct ECUData {
  int rpm = 0;
  float tpsv = 0;
  int tpsp = 0;
  float eotv = 0;
  int eotc = 0;
  float ectv = 0;
  int ectc = 0;
  int iatv = 0;
  int iatc = 0;
  int mapv = 0;
  int mapk = 0;
  float batv = 0;
  float injj = 0;
  float ckpp = 0;
  int spdk = 0;
  float oxigenv = 0;
  float oxigens = 0;
  bool valid = false;
  
  unsigned long totalPackets = 0;
  unsigned long errorPackets = 0;
  unsigned long lastUpdateTime = 0;
} liveECU;

// ==================== PROTOCOL DATA ====================
byte ECU_WAKEUP_MESSAGE[] = {0xFE, 0x04, 0x72, 0x8C};
byte ECU_INIT_MESSAGE[] = {0x72, 0x05, 0x00, 0xF0, 0x99};

// ==================== HELPER FUNCTIONS ====================
void forceGpioRelease(int pin) {
  gpio_reset_pin((gpio_num_t)pin);
  gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
  gpio_set_pull_mode((gpio_num_t)pin, GPIO_FLOATING);
  delay(10);
}

void nukeUART() {
  while(KLine.available()) KLine.read();
  KLine.flush();
  KLine.end();
  uart_driver_delete(UART_NUM_2);
  forceGpioRelease(KLINE_TX_PIN);
  forceGpioRelease(KLINE_RX_PIN);
  delay(50);
}

uint8_t calcChecksum(const uint8_t* data, uint8_t len) {
  uint8_t cksum = 0;
  for (uint8_t i = 0; i < len; i++) cksum -= data[i];
  return cksum;
}

// ==================== CONVERSION FUNCTIONS ====================
float calcValueDivide256(float val) { return (val * 5) / 256; }
float calcValueMinus40(int val) { return val - 40; }
float calcValueDivide10(float val) { return val / 10; }
float ckp(float val) { return (val * 265.5) / 65.535; }
float inj(float val) { return (val) / (65535 * 265.5); }

// ==================== INIT PULSE ====================
int initComms() {
  // CRITICAL: Hardware Anda pakai NPN transistor low-side switch
  // GPIO HIGH = Transistor ON = K-Line ditarik ke GND (0V)
  // GPIO LOW = Transistor OFF = K-Line pull-up ke 12V
  
  pinMode(KLINE_TX_PIN, OUTPUT);
  
  // PULSE 1: K-Line LOW (0V) selama 70ms
  digitalWrite(KLINE_TX_PIN, HIGH);  // Transistor ON
  delay(70);
  digitalWrite(KLINE_TX_PIN, LOW);   // Transistor OFF → K-Line HIGH
  delay(130);
  
  delay(50);
  
  // PULSE 2: K-Line LOW (0V) lagi
  digitalWrite(KLINE_TX_PIN, HIGH);
  delay(70);
  digitalWrite(KLINE_TX_PIN, LOW);
  delay(130);
  
  return 1;
}

// ✅ FIX: Unified echo flush function
void flushEcho(int expectedBytes) {
  int count = 0;
  unsigned long timeout = millis();
  
  while(count < expectedBytes && (millis() - timeout < 100)) {
    if(KLine.available()) {
      KLine.read();
      count++;
    }
  }
  
  // Flush any extra garbage
  delay(10);
  while(KLine.available()) KLine.read();
}

// ==================== ECU CONNECTION ====================
bool bangunkan() {
  nukeUART();
  delay(100);
  
  initComms();
  
  // RX inversion ON untuk kompensasi BC547 di RX path
  // TX tidak perlu inversion karena hardware sudah inverted
  KLine.begin(10400, SERIAL_8N1, KLINE_RX_PIN, KLINE_TX_PIN, true);
  
  delay(100);
  
  KLine.write(ECU_WAKEUP_MESSAGE, sizeof(ECU_WAKEUP_MESSAGE));
  delay(200);
  
  KLine.write(ECU_INIT_MESSAGE, sizeof(ECU_INIT_MESSAGE));
  KLine.flush();
  delay(50);
  
  int initBuffCount = 0;
  byte initBuff[32];
  
  while (KLine.available() > 0 && initBuffCount < 32) {
    initBuff[initBuffCount++] = KLine.read();
  }
  
  addLog(">>> ECU Connection Attempt");
  
  if (initBuffCount < 1) {
    addLog("  ✗ No response");
    return false;
  }
  
  String rs = "  RX (" + String(initBuffCount) + " bytes): ";
  for(int i=0; i<initBuffCount && i<10; i++) {
    if(initBuff[i] < 0x10) rs += "0";
    rs += String(initBuff[i], HEX) + " ";
  }
  addLog(rs);
  addLog("  ✓ Connected");
  
  connectedProtocol = "HONDA_K-LINE";
  
  return true;
}

// ==================== ECU MODE DETECTION ====================
void ecu_detect11() {
  byte data[] = {0x72, 0x05, 0x71, 0x11};
  byte chk = calcChecksum(data, sizeof(data));
  byte dataWithChk[] = {0x72, 0x05, 0x71, 0x11, chk};
  
  while(KLine.available()) KLine.read();
  
  KLine.write(dataWithChk, sizeof(dataWithChk));
  delay(60);  // Tunggu TX selesai
  
  flushEcho(5);  // Buang echo 5 bytes
  
  int buffCount = 0;
  byte buff[PACKET_BUFFER_SIZE];
  unsigned long timeout = millis();
  
  while ((millis() - timeout < 200) && (buffCount < PACKET_BUFFER_SIZE)) {
    if(KLine.available()) {
      buff[buffCount++] = KLine.read();
    }
  }
  
  addLog("  Mode 0x11: " + String(buffCount) + " bytes");
  
  if (buffCount == 30) {
    ecuMode = 11;
    addLog("    ✓ Mode 0x11 detected (standard)");
  } else if (buffCount == 29) {
    ecuMode = 71;
    addLog("    ✓ Mode 0x11 detected (sport variant)");
  }
}

void ecu_detect17() {
  if(ecuMode != 0) return;
  
  byte data[] = {0x72, 0x05, 0x71, 0x17};
  byte chk = calcChecksum(data, sizeof(data));
  byte dataWithChk[] = {0x72, 0x05, 0x71, 0x17, chk};
  
  while(KLine.available()) KLine.read();
  
  KLine.write(dataWithChk, sizeof(dataWithChk));
  delay(60);
  
  flushEcho(5);
  
  int buffCount = 0;
  byte buff[PACKET_BUFFER_SIZE];
  unsigned long timeout = millis();
  
  while ((millis() - timeout < 200) && (buffCount < PACKET_BUFFER_SIZE)) {
    if(KLine.available()) {
      buff[buffCount++] = KLine.read();
    }
  }
  
  addLog("  Mode 0x17: " + String(buffCount) + " bytes");
  
  if (buffCount == 28) {
    ecuMode = 17;
    addLog("    ✓ Mode 0x17 detected (standard)");
  } else if (buffCount == 29) {
    ecuMode = 173;
    addLog("    ✓ Mode 0x17 detected (ESP variant)");
  }
}

void ecu_detect10() {
  if(ecuMode != 0) return;
  
  byte data[] = {0x72, 0x05, 0x71, 0x10};
  byte chk = calcChecksum(data, sizeof(data));
  byte dataWithChk[] = {0x72, 0x05, 0x71, 0x10, chk};
  
  while(KLine.available()) KLine.read();
  
  KLine.write(dataWithChk, sizeof(dataWithChk));
  delay(60);
  
  flushEcho(5);
  
  int buffCount = 0;
  byte buff[PACKET_BUFFER_SIZE];
  unsigned long timeout = millis();
  
  while ((millis() - timeout < 200) && (buffCount < PACKET_BUFFER_SIZE)) {
    if(KLine.available()) {
      buff[buffCount++] = KLine.read();
    }
  }
  
  addLog("  Mode 0x10: " + String(buffCount) + " bytes");
  
  if (buffCount == 27) {
    ecuMode = 10;
    addLog("    ✓ Mode 0x10 detected (sport)");
  }
}

// ==================== READ DATA WITH UNIFIED ECHO HANDLING ====================
void data_11() {
  byte request[] = {0x72, 0x05, 0x71, 0x11};
  byte chk = calcChecksum(request, sizeof(request));
  byte dataWithChk[] = {0x72, 0x05, 0x71, 0x11, chk};
  
  while(KLine.available()) KLine.read();
  
  KLine.write(dataWithChk, sizeof(dataWithChk));
  delay(60);
  
  flushEcho(5);  // ✅ Consistent echo handling
  
  int buffCount = 0;
  byte buff[PACKET_BUFFER_SIZE];
  unsigned long readTimeout = millis();
  
  while (millis() - readTimeout < 150) {
    if (KLine.available()) {
      buff[buffCount++] = KLine.read();
      if (buffCount >= PACKET_BUFFER_SIZE) break;
    }
  }

  int start = -1;
  for(int i = 0; i < (buffCount - 1); i++) {
    if(buff[i] == 0x02 && (buff[i+1] == 0x1D || buff[i+1] == 0x1C)) {
      start = i;
      break;
    }
  }

  if(start != -1 && (buffCount - start) >= 20) {
    liveECU.rpm = (buff[start + 10] * 256) + buff[start + 11];
    liveECU.tpsp = (buff[start + 12] * 100) / 255;
    liveECU.ectc = buff[start + 13] - 40;
    liveECU.batv = buff[start + 16] / 10.0;
    liveECU.spdk = buff[start + 17];

    liveECU.valid = true;
    liveECU.totalPackets++;
    liveECU.lastUpdateTime = millis();
    lastValidData = millis();  // ✅ Update watchdog
    lastSuccessfulPoll = millis();
  } else {
    liveECU.errorPackets++;
    liveECU.valid = false;
  }
}

void data_17() {
  byte data[] = {0x72, 0x05, 0x71, 0x17};
  byte chk = calcChecksum(data, sizeof(data));
  byte dataWithChk[] = {0x72, 0x05, 0x71, 0x17, chk};
  
  while(KLine.available()) KLine.read();
  
  KLine.write(dataWithChk, sizeof(dataWithChk));
  delay(60);
  
  flushEcho(5);  // ✅ FIX: Tambah echo handling yang hilang
  
  int buffCount = 0;
  byte buff[PACKET_BUFFER_SIZE];
  unsigned long timeout = millis();
  
  while ((millis() - timeout < 150) && (buffCount < PACKET_BUFFER_SIZE)) {
    if(KLine.available()) {
      buff[buffCount++] = KLine.read();
    }
  }
  
  liveECU.totalPackets++;
  
  if(buffCount >= 22) {
    liveECU.rpm = (buff[9] * 100);
    liveECU.tpsp = (buff[10] * 100) / 255;
    liveECU.ectc = buff[11] - 40;
    liveECU.iatc = buff[12] - 40;
    liveECU.mapk = buff[13];
    liveECU.batv = buff[14] / 10.0;
    liveECU.spdk = buff[15];
    
    liveECU.injj = inj((buff[18] * 256) + buff[19]);
    liveECU.ckpp = ckp((buff[20] * 256) + buff[21]);
    
    liveECU.valid = true;
    liveECU.lastUpdateTime = millis();
    lastValidData = millis();  // ✅ Update watchdog
    lastSuccessfulPoll = millis();
  } else {
    liveECU.errorPackets++;
    liveECU.valid = false;
  }
}

void data_10() {
  byte data[] = {0x72, 0x05, 0x71, 0x10};
  byte chk = calcChecksum(data, sizeof(data));
  byte dataWithChk[] = {0x72, 0x05, 0x71, 0x10, chk};
  
  while(KLine.available()) KLine.read();
  
  KLine.write(dataWithChk, sizeof(dataWithChk));
  delay(60);
  
  flushEcho(5);  // ✅ FIX: Tambah echo handling yang hilang
  
  int buffCount = 0;
  byte buff[PACKET_BUFFER_SIZE];
  unsigned long timeout = millis();
  
  while ((millis() - timeout < 150) && (buffCount < PACKET_BUFFER_SIZE)) {
    if(KLine.available()) {
      buff[buffCount++] = KLine.read();
    }
  }
  
  liveECU.totalPackets++;
  
  if(buffCount >= 17) {
    liveECU.rpm = (buff[9] * 100);
    liveECU.tpsp = (buff[10] * 100) / 255;
    liveECU.ectc = buff[12] - 40;
    liveECU.iatc = buff[13] - 40;
    liveECU.mapk = buff[14];
    liveECU.batv = buff[15] / 10.0;
    liveECU.spdk = buff[16];
    
    liveECU.valid = true;
    liveECU.lastUpdateTime = millis();
    lastValidData = millis();  // ✅ Update watchdog
    lastSuccessfulPoll = millis();
  } else {
    liveECU.errorPackets++;
    liveECU.valid = false;
  }
}

// ✅ FIX: Reconnect dengan proper watchdog reset
bool attemptReconnect() {
  addLog(">>> Auto-reconnect triggered");
  
  ecuConnected = false;
  ecuMode = 0;
  liveECU.valid = false;
  
  if(bangunkan()) {
    ecuConnected = true;
    
    delay(2000);
    
    ecu_detect11();
    delay(200);
    ecu_detect17();
    delay(200);
    ecu_detect10();
    
    if(ecuMode != 0) {
      connectedProtocol = "HONDA_MODE_0x" + String(ecuMode, HEX);
      addLog("  ✓ Reconnected! Mode: 0x" + String(ecuMode, HEX));
      
      // ✅ CRITICAL FIX: Reset ALL watchdog timers
      lastValidData = millis();
      lastSuccessfulPoll = millis();
      connectionFailCount = 0;
      
      return true;
    }
  }
  
  connectionFailCount++;
  addLog("  ✗ Reconnect failed (" + String(connectionFailCount) + "/" + String(MAX_FAIL_BEFORE_REBOOT) + ")");
  
  return false;
}

// ==================== WEB HANDLERS ====================
void handleRoot() { 
  server.send(200, "text/html", PAGE_HTML); 
}

void handleLogs() { 
  server.send(200, "text/plain", getLogs()); 
}

void handleData() {
  String json = "{";
  json += "\"connected\":" + String(ecuConnected ? "true" : "false") + ",";
  json += "\"mode\":\"0x" + String(ecuMode, HEX) + "\",";
  json += "\"rpm\":" + String(liveECU.rpm) + ",";
  json += "\"tps\":" + String(liveECU.tpsp) + ",";
  json += "\"temp\":" + String(liveECU.ectc) + ",";
  json += "\"speed\":" + String(liveECU.spdk) + ",";
  json += "\"battery\":" + String(liveECU.batv) + ",";
  json += "\"valid\":" + String(liveECU.valid ? "true" : "false") + ",";
  json += "\"totalPackets\":" + String(liveECU.totalPackets) + ",";
  json += "\"errorPackets\":" + String(liveECU.errorPackets) + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";
  server.send(200, "application/json", json);
}

void handleStatus() {
  String json = "{";
  json += "\"connected\":" + String(ecuConnected ? "true" : "false") + ",";
  json += "\"protocol\":\"" + connectedProtocol + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleReconnect() {
  if(attemptReconnect()) {
    server.send(200, "text/plain", "OK");
  } else {
    server.send(500, "text/plain", "FAIL");
  }
}

void handleNotFound() { 
  server.send(404, "text/plain", "Not Found"); 
}

void handleUpdatePage() { 
  server.send(200, "text/html", PAGE_OTA_HTML); 
}

void handleDoUpdate() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    addLog("OTA: " + upload.filename);
    if (!Update.begin()) {
      Update.printError(Serial);
    }
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    Update.write(upload.buf, upload.currentSize);
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      addLog("OTA SUCCESS");
    } else {
      Update.printError(Serial);
    }
  }
}

void handleUpdateFinish() {
  if (Update.hasError()) {
    server.send(500, "text/plain", "FAIL");
  } else {
    server.send(200, "text/plain", "OK\nRebooting...");
    delay(2500);
    ESP.restart();
  }
}

// ==================== SETUP ====================
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  Serial.begin(115200);
  delay(200);
  
  addLog("════════════════════════════════");
  addLog("Honda OBD Scanner v13.0");
  addLog("════════════════════════════════");
  
  uart_driver_delete(UART_NUM_2);
  forceGpioRelease(KLINE_TX_PIN);
  forceGpioRelease(KLINE_RX_PIN);
  addLog("✓ GPIO init");
  
  WiFi.mode(WIFI_AP);
  delay(200);
  WiFi.softAPConfig(localIP, gateway, subnet);
  WiFi.softAP(ap_ssid, ap_pass);
  addLog("✓ AP: " + String(ap_ssid));
  addLog("✓ IP: " + WiFi.softAPIP().toString());
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/logs", HTTP_GET, handleLogs);
  server.on("/data", HTTP_GET, handleData);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/reconnect", HTTP_POST, handleReconnect);
  server.on("/update", HTTP_GET, handleUpdatePage);
  server.on("/update", HTTP_POST, handleUpdateFinish, handleDoUpdate);
  server.onNotFound(handleNotFound);
  server.begin();
  
  addLog("✓ Web: http://192.168.4.1/");
  addLog("════════════════════════════════");
  
  addLog("Connecting to ECU...");
  delay(1000);
  
  if(bangunkan()) {
    ecuConnected = true;
    digitalWrite(LED_PIN, HIGH);
    
    addLog("Waiting 4 seconds for ECU ready...");
    delay(4000);
    
    ecu_detect11();
    delay(200);
    ecu_detect17();
    delay(200);
    ecu_detect10();
    
    if(ecuMode != 0) {
      connectedProtocol = "HONDA_MODE_0x" + String(ecuMode, HEX);
      addLog("✓ Mode: 0x" + String(ecuMode, HEX));
      addLog("✓ Ready!");
      
      // ✅ CRITICAL: Initialize watchdog timers on successful connect
      lastValidData = millis();
      lastSuccessfulPoll = millis();
    } else {
      addLog("✗ Mode detection failed");
      ecuConnected = false;
      digitalWrite(LED_PIN, LOW);
    }
  } else {
    addLog("✗ ECU connection failed");
    ecuConnected = false;
    digitalWrite(LED_PIN, LOW);
  }
}

// ==================== LOOP ====================
unsigned long lastDataRequest = 0;
const unsigned long DATA_INTERVAL = 500;

void loop() {
  server.handleClient();
  
  // ✅ FIX: Reconnect logic dengan proper timing check
  if(!ecuConnected) {
    unsigned long now = millis();
    if(now - lastConnectionAttempt >= RECONNECT_INTERVAL) {
      lastConnectionAttempt = now;
      
      if(attemptReconnect()) {
        digitalWrite(LED_PIN, HIGH);
      } else {
        if(connectionFailCount >= MAX_FAIL_BEFORE_REBOOT) {
          addLog("!!! Too many failures, rebooting...");
          delay(2000);
          ESP.restart();
        }
      }
    }
    
    digitalWrite(LED_PIN, LOW);
    return;
  }
  
  // ✅ FIX: Watchdog hanya trigger jika benar-benar tidak ada data
  // DAN sudah lewat timeout periode
  if(ecuConnected && (millis() - lastValidData > DATA_TIMEOUT)) {
    // Additional check: pastikan kita sudah coba poll minimal 1x
    if(millis() - lastSuccessfulPoll > POLL_TIMEOUT) {
      addLog("!!! Data timeout after " + String(DATA_TIMEOUT/1000) + "s, reconnecting...");
      ecuConnected = false;
      digitalWrite(LED_PIN, LOW);
      return;
    }
  }
  
  // Normal data polling loop
  if (ecuConnected && ecuMode != 0) {
    unsigned long now = millis();
    if (now - lastDataRequest >= DATA_INTERVAL) {
      lastDataRequest = now;
      
      if(ecuMode == 11 || ecuMode == 71) { 
        data_11();
      } else if(ecuMode == 17 || ecuMode == 173) {
        data_17();
      } else if(ecuMode == 10) {
        data_10();
      }
      
      if(liveECU.valid) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      }
    }
  }
}
