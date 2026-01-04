// Sketch untuk ESP32-C3 sebagai Bridge
void setup() {
  // Serial adalah USB CDC ke HP Android
  Serial.begin(115200); 
  
  // Serial1 adalah jalur ke ESP32 yang rusak
  // RX=20, TX=21
  Serial1.begin(115200, SERIAL_8N1, 20, 21); 
}

void loop() {
  // HP -> ESP32-C3 -> ESP32 Rusak
  if (Serial.available()) {
    Serial1.write(Serial.read());
  }
  // ESP32 Rusak -> ESP32-C3 -> HP
  if (Serial1.available()) {
    Serial.write(Serial1.read());
  }
}
