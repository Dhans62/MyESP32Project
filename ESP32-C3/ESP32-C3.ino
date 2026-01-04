// Serial Passthrough untuk ESP32-C3
// Menggunakan USB CDC internal untuk bicara ke HP
// Menggunakan Hardware Serial (UART1) di pin 20 & 21 untuk bicara ke ESP32 Rusak

void setup() {
  Serial.begin(115200); // Koneksi USB ke Android
  // UART1: RX=20, TX=21
  Serial1.begin(115200, SERIAL_8N1, 20, 21); 
}

void loop() {
  // Teruskan data dari HP ke ESP32 Rusak
  while (Serial.available()) {
    Serial1.write(Serial.read());
  }
  // Teruskan respon dari ESP32 Rusak ke HP
  while (Serial1.available()) {
    Serial.write(Serial1.read());
  }
}
