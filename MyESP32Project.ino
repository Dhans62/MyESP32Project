#include "ble_ota.h"

void setup() {
    Serial.begin(115200);
    Serial.println("Memulai Sistem dengan Core 3.0.7");
    setupBLEOTA();
    Serial.println("BLE OTA Siap Menunggu Koneksi...");
}

void loop() {
    // Logika utama Anda di sini
    delay(1000);
}

