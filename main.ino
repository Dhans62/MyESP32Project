#include <Arduino.h>
#include <Update.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// UUID standar untuk Service dan Characteristic OTA
#define SERVICE_UUID "fb1e4001-54aa-4b8b-9a35-0d605658d551"
#define CHARACTERISTIC_UUID "fb1e4002-54aa-4b8b-9a35-0d605658d551"

void setupBLEOTA() {
    BLEDevice::init("ESP32_OTA_UPDATE");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    // Di sini Anda bisa menambahkan logika Update.write() saat data diterima
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();
}
