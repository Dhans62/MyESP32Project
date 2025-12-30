#include <HardwareSerial.h>

// Gunakan UART1 (Default ESP32-C3 RX=20, TX=21)
HardwareSerial testSerial(1); 

uint32_t baudRates[] = {10400, 19200, 38400, 115200, 250000};
int currentIdx = 0;
unsigned long lastUpdate = 0;
unsigned long totalSent = 0;
unsigned long totalCorrect = 0;

void setup() {
  Serial.begin(115200); // Ke Arduinodroid
  delay(3000);
  
  // Memulai tes awal
  testSerial.begin(baudRates[currentIdx], SERIAL_8N1, 20, 21);
  Serial.printf("\n--- START TEST: %d BAUD ---\n", baudRates[currentIdx]);
}

void loop() {
  // 1. Stress Test: Kirim data sebanyak mungkin dalam loop tercepat
  uint8_t txByte = 0xAA;
  testSerial.write(txByte);
  totalSent++;

  // Cek balikan (Echo)
  if (testSerial.available()) {
    if (testSerial.read() == txByte) {
      totalCorrect++;
    }
  }

  // 2. Reporting: Hanya kirim log ke Arduinodroid setiap 1 detik
  if (millis() - lastUpdate > 1000) {
    float accuracy = (totalSent > 0) ? ((float)totalCorrect / totalSent) * 100.0 : 0;
    
    // Kirim satu baris saja ke terminal
    Serial.printf("[%lu] Baud: %d | Acc: %.2f%% | Samples: %lu\n", 
                  millis()/1000, baudRates[currentIdx], accuracy, totalSent);

    lastUpdate = millis();

    // Ganti baud rate setiap 10 detik untuk mencari limit hardware
    static int switchCounter = 0;
    switchCounter++;
    if (switchCounter >= 10) {
      switchCounter = 0;
      currentIdx++;
      if (currentIdx >= 5) currentIdx = 0;
      
      totalSent = 0;
      totalCorrect = 0;
      testSerial.end();
      delay(500);
      testSerial.begin(baudRates[currentIdx], SERIAL_8N1, 20, 21);
      Serial.printf("\n>>> SWITCHING TO %d BAUD <<<\n", baudRates[currentIdx]);
    }
  }
}
