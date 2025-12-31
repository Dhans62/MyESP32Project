#include <HardwareSerial.h>

// Pada ESP32-C3, UART1 adalah pilihan terbaik untuk pengetesan hardware
HardwareSerial testSerial(1); 

uint32_t baudRates[] = {10400, 19200, 38400, 115200, 250000};
int currentIdx = 0;
unsigned long lastUpdate = 0;
unsigned long totalSent = 0;
unsigned long totalCorrect = 0;
unsigned long startTime = 0;

void setup() {
  // Serial utama untuk log ke Arduinodroid via USB-OTG
  Serial.begin(115200); 
  delay(5000); // Memberi waktu Anda membuka Serial Monitor di HP
  
  Serial.println("\n========================================");
  Serial.println("ESP32-C3 6N137 HARDWARE BENCHMARK");
  Serial.println("========================================");

  startNewBaudTest();
}

void startNewBaudTest() {
  uint32_t baud = baudRates[currentIdx];
  
  // Memulai UART1 dengan pin default C3 (RX:20, TX:21)
  testSerial.begin(baud, SERIAL_8N1, 20, 21);
  
  // KRUSIAL: Inversi Hardware untuk 6N137 (Inverting Opto)
  // Tanpa ini, hasil akan selalu Acc: 0.0%
  testSerial.setRxInvert(true);
  testSerial.setTxInvert(true);
  
  totalSent = 0;
  totalCorrect = 0;
  startTime = millis();
  
  Serial.printf("\n>>> TESTING @ %d BAUD <<<\n", baud);
}

void loop() {
  // 1. Kirim byte stress test (0xAA = 10101010)
  uint8_t txByte = 0xAA;
  testSerial.write(txByte);
  totalSent++;

  // 2. Baca Echo (Loopback)
  // Berikan timeout singkat 5ms agar tidak stuck jika kabel lepas
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

  // 3. Update Laporan ke Arduinodroid setiap 1 detik (Batching)
  if (millis() - lastUpdate > 1000) {
    float accuracy = (totalSent > 0) ? ((float)totalCorrect / totalSent) * 100.0 : 0;
    
    // Format log ringan agar tidak membuat terminal Android lag
    Serial.printf("Baud:%-7d | Acc:%-6.2f%% | S:%lu\n", 
                  baudRates[currentIdx], accuracy, totalSent);

    lastUpdate = millis();

    // Ganti Baud Rate otomatis setiap 15 detik
    if (millis() - startTime > 15000) {
      currentIdx++;
      if (currentIdx >= 5) currentIdx = 0; // Reset ke 10400
      
      testSerial.end();
      delay(200);
      startNewBaudTest();
    }
  }
}
