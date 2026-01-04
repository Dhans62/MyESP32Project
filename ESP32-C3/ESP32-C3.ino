void setup() {
  // Tunggu USB siap. Ini krusial di Android!
  Serial.begin(115200); 
  delay(2000); 

  // Paksa pin 20 dan 21 jadi UART, bukan JTAG/USB
  // RX = 20, TX = 21
  Serial1.begin(115200, SERIAL_8N1, 20, 21);
  
  // Kasih tanda kalau C3 sudah hidup
  Serial.println("C3 BRIDGE READY...");
}

void loop() {
  // HP -> C3 -> Target
  if (Serial.available()) {
    int data = Serial.read();
    Serial1.write(data);
  }
  
  // Target -> C3 -> HP
  if (Serial1.available()) {
    int data = Serial1.read();
    Serial.write(data);
  }
}
