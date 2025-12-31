#include <HardwareSerial.h>

HardwareSerial testSerial(1); 
#define LED_PIN 8 // Sesuaikan dengan pin LED board C3 Anda
#define RX_PIN 20
#define TX_PIN 21

void setup() {
  pinMode(LED_PIN, OUTPUT);
  // Mulai dengan baud standar motor 10400
  // Parameter 'true' untuk membalik logika 6N137
  testSerial.begin(10400, SERIAL_8N1, RX_PIN, TX_PIN, true);
}

void loop() {
  uint8_t txByte = 0xAA;
  testSerial.write(txByte);
  
  delay(10); // Memberi waktu proses

  if (testSerial.available()) {
    if (testSerial.read() == txByte) {
      // DATA BENAR - LED NYALA TERUS
      digitalWrite(LED_PIN, HIGH); 
    } else {
      // DATA SALAH/CORRUPT - LED KEDIP CEPAT
      digitalWrite(LED_PIN, HIGH);
      delay(50);
      digitalWrite(LED_PIN, LOW);
      delay(50);
    }
  } else {
    // TIDAK ADA ECHO - LED KEDIP LAMBAT
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }
}
