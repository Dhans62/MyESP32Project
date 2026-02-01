#include <Arduino.h>
#include <HardwareSerial.h>

// DEFINISI PIN UNTUK ESP32-C3
#define KLINE_RX 5
#define KLINE_TX 6

// Gunakan Serial1 agar Serial0 tetap untuk Debugging/Laptop
HardwareSerial KLine(1); 

enum State { RESET_BUS, SEARCHING, AUTHORIZING, READING_DATA };
State currentState = RESET_BUS;

unsigned long lastResponse = 0, lastReq = 0, lastDashUpdate = 0;

void setup() {
  // Debugging via USB CDC (Monitor Laptop)
  Serial.begin(115200);
  
  // Inisialisasi pin TX secara manual untuk sinyal Wake-up
  pinMode(KLINE_TX, OUTPUT);
  digitalWrite(KLINE_TX, HIGH);

  Serial.println("\n[SYSTEM] ESP32-C3 K15 DECODER INITIALIZED");
  Serial.printf("[INFO] RX: GPIO %d | TX: GPIO %d\n", KLINE_RX, KLINE_TX);
}

void displayDashboard(byte* d) {
  // 1. DATA DASAR
  int rpm = (d[4] * 256) + d[5];
  int ect = d[9] - 40;  
  int iat = d[11] - 40; 
  
  if (iat < -30 && rpm > 0) return; // Abaikan jika data korup

  // 2. TPS MAPPING (Berdasarkan kalibrasi 0% = 5632 | 100% = 56474)
  long tpsRaw = (d[6] * 256) + d[7];
  float tpsMvol = tpsRaw * 0.076; 
  float tpsPct = (float)(tpsRaw - 5632) * 100.0 / (56474 - 5632);
  tpsPct = constrain(tpsPct, 0, 100);

  // 3. FUEL & AIR
  float injMs = ((d[12] * 256) + d[13]) / 1000.0;
  if (rpm < 400) injMs = 0.0; 

  int iscStep = d[8];
  float o2Volts = d[23] * 0.005;

  // 4. ECU STATUS
  String modeDesc;
  switch(d[18]) {
    case 0x00: modeDesc = "OPEN LOOP"; break;
    case 0x02: modeDesc = "CLOSED LOOP"; break;
    case 0x03: modeDesc = "ACCEL ENRICH"; break;
    case 0x04: modeDesc = "DECEL CUT"; break;
    default:   modeDesc = "MODE:" + String(d[18]);
  }

  // OUTPUT DASHBOARD
  Serial.println("\n=============================================");
  Serial.printf(" RPM : %-5d  |  ECT : %-2d °C  |  IAT : %-2d °C\n", rpm, ect, iat);
  Serial.printf(" INJ : %-4.2f ms |  ISC : %-3d stp |  O2  : %-4.2f V\n", injMs, iscStep, o2Volts);
  Serial.printf(" TPS : %-4.1f %%  |  TPS : %-4.0f mV |  %s\n", tpsPct, tpsMvol, modeDesc.c_str());
  
  if (d[14] == 0xFF && d[15] == 0xFF) {
    Serial.println(" SYSTEM STATUS: NO ERROR (CLEAN)");
  } else {
    Serial.printf(" !!! WARNING DTC DETECTED: %02X %02X !!!\n", d[14], d[15]);
  }
  Serial.println("=============================================");
}

void loop() {
  switch (currentState) {
    case RESET_BUS:
      KLine.end();
      pinMode(KLINE_TX, OUTPUT);
      digitalWrite(KLINE_TX, HIGH);
      delay(2000);
      currentState = SEARCHING;
      break;

    case SEARCHING:
      // K-Line Wake-up Pattern
      digitalWrite(KLINE_TX, LOW);  delay(70);
      digitalWrite(KLINE_TX, HIGH); delay(120);
      
      // Khusus C3: Inisialisasi Serial1 pada GPIO 5 & 6
      KLine.begin(10400, SERIAL_8N1, KLINE_RX, KLINE_TX);
      
      {
        byte wk[] = {0xFE, 0x04, 0x72, 0x8C};
        KLine.write(wk, 4); 
        delay(300);
        if (KLine.available()) currentState = AUTHORIZING;
        else currentState = RESET_BUS;
      }
      break;

    case AUTHORIZING:
      {
        byte au[] = {0x72, 0x05, 0x00, 0xF0, 0x99};
        while(KLine.available()) KLine.read();
        KLine.write(au, 5); 
        delay(300);
        if (KLine.available()) {
          currentState = READING_DATA;
          lastResponse = millis();
        } else currentState = RESET_BUS;
      }
      break;

    case READING_DATA:
      // Request Data setiap 150ms
      if (millis() - lastReq > 150) {
        byte cmd[] = {0x72, 0x05, 0x71, 0x11, 0x07};
        while(KLine.available()) KLine.read(); 
        KLine.write(cmd, 5);
        lastReq = millis();
      }

      // Read 25 Byte Response
      if (KLine.available() >= 25) {
        byte b[64];
        int len = KLine.readBytes(b, 64);
        int s = -1;
        for (int i = 0; i <= len - 25; i++) {
          if (b[i] == 0x02 && b[i+1] == 0x19) { s = i; break; }
        }

        if (s != -1) {
          lastResponse = millis();
          if (millis() - lastDashUpdate > 350) {
            displayDashboard(&b[s]);
            lastDashUpdate = millis();
          }
        }
      }

      // Reconnect jika ECU diam lebih dari 4 detik
      if (millis() - lastResponse > 4000) {
        Serial.println("[!] ECU TIMEOUT - RESTARTING BUS...");
        currentState = RESET_BUS;
      }
      break;
  }
}
