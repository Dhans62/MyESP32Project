// Gunakan GPIO 8 untuk ESP32-C3 standar
// Jika LED tidak berkedip, coba ganti ke GPIO 2 atau 10
const int LED_PIN = 8; 

void setup() {
  // Inisialisasi Serial untuk debug lewat HP
  Serial.begin(115200);
  
  // Konfigurasi Pin
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("ESP32-C3 Ready!");
  Serial.print("Blinking on GPIO: ");
  Serial.println(LED_PIN);
}

void loop() {
  // Nyalakan LED
  digitalWrite(LED_PIN, HIGH);
  Serial.println("Status: HIGH");
  delay(1000); // Delay 1 detik
  
  // Matikan LED
  digitalWrite(LED_PIN, LOW);
  Serial.println("Status: LOW");
  delay(1000); 
}
