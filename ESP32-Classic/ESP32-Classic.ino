#include <HardwareSerial.h>

// Definisi Pin
#define TX_PIN 17
#define RX_PIN 16

// Inisialisasi Serial untuk ECU pada UART2
HardwareSerial bike(2);

void setup() {
  // Serial Monitor ke Android/PC
  Serial.begin(115200);
  
  // Inisialisasi awal Pin TX sebagai Output HIGH agar K-Line tetap Idle di 12V
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, HIGH);
  
  Serial.println("\n========================================");
  Serial.println("K15 ATTACK V4.1");
  Serial.println("========================================");
  Serial.println("Ketik 'G' dan putar Kontak ke ON bersamaan...");
}

void executeHandshake() {
  Serial.println("\n[1] Memulai Physical Wakeup (Double Pulse)...");
  
  // Matikan Serial agar bisa melakukan Bit-Banging
  bike.end();
  delay(10);
  
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, HIGH);
  delay(50); // Pastikan tegangan stabil di 12V sebelum pulsa dimulai

  // Kirim pulsa Wakeup (Protokol KWP2000 Pro)
  for(int i = 0; i < 2; i++) {
    digitalWrite(TX_PIN, LOW);  delay(70);
    digitalWrite(TX_PIN, HIGH); delay(130);
  }
  
  // Jeda P3 (Inter-message time) sesuai standar ISO 14230-2
  delay(60); 
  
  // Aktifkan UART kembali dengan Baudrate Honda (10400)
  bike.begin(10400, SERIAL_8N1, RX_PIN, TX_PIN);
  
  // Beri waktu UART untuk stabil di Core 2.0.7
  delay(100); 
  
  // Bersihkan buffer dari sinyal liar/noise saat transisi
  while(bike.available()) bike.read();

  // --- LANGKAH 2: KIRIM INIT MESSAGE ---
  // Kita coba alamat 0x11 (Engine) sesuai referensi K15
  byte initMsg[] = {0x72, 0x05, 0x00, 0x11, 0x78};
  
  Serial.println("[2] Mengirim StartCommunication (0x11)...");
  bike.write(initMsg, 5);
  
  // Tunggu respon ECU
  unsigned long startTime = millis();
  bool gotResponse = false;
  
  while (millis() - startTime < 1000) { // Tunggu hingga 1 detik
    if (bike.available()) {
      byte r = bike.read();
      Serial.printf("%02X ", r);
      gotResponse = true;
    }
  }

  if (!gotResponse) {
    Serial.println("\n[!] ECU Diam. Mencoba Alamat Broadcast (0xF0)...");
    byte broadcastMsg[] = {0x72, 0x05, 0x00, 0xF0, 0x99};
    bike.write(broadcastMsg, 5);
    
    startTime = millis();
    while (millis() - startTime < 1000) {
      if (bike.available()) {
        Serial.printf("%02X ", bike.read());
      }
    }
  }
  Serial.println("\n--- Selesai ---");
}

void loop() {
  // Cek input dari Serial Monitor
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'G' || cmd == 'g') {
      executeHandshake();
    }
  }
}

