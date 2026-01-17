#include <HardwareSerial.h>
#define TX_PIN 17
#define RX_PIN 16
HardwareSerial bike(2);

void setup() {
  Serial.begin(115200);
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, HIGH);
  Serial.println("\n[!] TARGET TERKUNCI: ISO 14230");
  Serial.println("[!] SOP: Kontak OFF -> Ketik 'G' -> Segera Kontak ON");
}

void attack() {
  bike.end();
  pinMode(TX_PIN, OUTPUT);
  
  // Ketukan Pintu (Physical Pulse)
  digitalWrite(TX_PIN, LOW);  delay(70);
  digitalWrite(TX_PIN, HIGH); delay(130);
  
  bike.begin(10400, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(100);
  
  // Membersihkan buffer sampah
  while(bike.available()) bike.read();

  // Kirim Start Communication ISO 14230
  // Format: [Format] [Target] [Source] [Action] [Checksum]
  byte isoInit[] = {0x81, 0x11, 0xF1, 0x81, 0x04};
  
  Serial.println("[1] Mengirim Start Communication...");
  bike.write(isoInit, 5);
  
  unsigned long start = millis();
  Serial.print("[2] Respon ECU: ");
  while(millis() - start < 1000) {
    if(bike.available()) {
      Serial.printf("%02X ", bike.read());
    }
  }
  Serial.println("\n-------------------------");
}

void loop() {
  if (Serial.available()) {
    if (Serial.read() == 'G') attack();
  }
}
