#include <HardwareSerial.h>
// Gunakan UART1 untuk pengujian hardware
HardwareSerial testSerial(1); 
// Pin Default ESP32-C3 untuk UART1
#define RX_PIN 20
#define TX_PIN 21

uint32_t baudRates[] = {10400, 19200, 38400, 115200, 250000};
int currentIdx = 0;
unsigned long lastUpdate = 0;
unsigned long totalSent = 0;
unsigned long totalCorrect = 0;
unsigned long startTime = 0;

void setup() {
  // Serial utama (USB) untuk log ke Arduinodroid
  Serial.begin(115200); 
  delay(5000); 
  
  Serial.println("\n========================================");
  Serial.println("ESP32-C3 6N137 HARDWARE BENCHMARK");
  Serial.println("========================================");

  startNewBaudTest();
}

void startNewBaudTest() {
  uint32_t baud = baudRates[currentIdx];
  
  // Solusi Kompilasi: Menggunakan parameter ke-5 (invert) = true
  // Ini akan mengaktifkan inversi hardware pada RX dan TX sekaligus
  // Sangat krusial untuk 6N137 agar tidak Acc: 0.0%
  testSerial.begin(baud, SERIAL_8N1, RX_PIN, TX_PIN, true);
  
  totalSent = 0;
  totalCorrect = 0;
  startTime = millis();
  
  Serial.printf("\n>>> TESTING @ %d BAUD (Inverted) <<<\n", baud);
}

void loop() {
  // 1. Stress Test: Kirim byte berpola 0xAA (10101010)
  uint8_t txByte = 0xAA;
  testSerial.write(txByte);
  totalSent++;

  // 2. Baca Echo (Loopback) dengan Timeout 5ms
  unsigned long timeout = micros();
  while (micros() - timeout < 5000) {
    if (testSerial.available()) {
      uint8_t rxByte = testSerial.read();
      if (rxByte == txByte) {
        totalCorrect++;
      }
      break; 
    }
  }

  // 3. Pelaporan Batch (1 detik sekali) agar Arduinodroid tidak crash
  if (millis() - lastUpdate > 1000) {
    float accuracy = (totalSent > 0) ? ((float)totalCorrect / totalSent) * 100.0 : 0;
    
    // Log minimalis dan jujur
    Serial.printf("Baud:%-7d | Acc:%-6.2f%% | Samples:%lu\n", 
                  baudRates[currentIdx], accuracy, totalSent);

    lastUpdate = millis();

    // Otomatis ganti Baud Rate setiap 15 detik
    if (millis() - startTime > 15000) {
      currentIdx++;
      if (currentIdx >= 5) currentIdx = 0;
      
      testSerial.end();
      delay(200);
      startNewBaudTest();
    }
  }
}
