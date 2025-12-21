# ESP32 Multi-Board Automated Compiler

Repositori ini adalah sistem kompilasi otomatis berbasis cloud menggunakan **GitHub Actions**. Sistem ini dirancang untuk menghasilkan file binary (.bin) untuk berbagai varian chip ESP32 tanpa memerlukan instalasi berat di perangkat lokal (HP/Termux).

## 🚀 Fitur Utama
- **Core Version**: ESP32 Arduino Core v3.0.7 (Stable).
- **Automated Build**: Kompilasi berjalan otomatis setiap kali kode di-push.
- **Multi-Platform**: Mendukung arsitektur Xtensa (ESP32, S3) dan RISC-V (C3).
- **BLE OTA Ready**: Kode dasar sudah mendukung fitur update firmware via Bluetooth.

## 🛠️ Board yang Didukung
| Board Name | FQBN | Arsitektur |
|------------|------|------------|
| ESP32 Dev Kit V1 / Generic | `esp32:esp32:esp32` | Xtensa LX6 |
| ESP32-S3 | `esp32:esp32:esp32s3` | Xtensa LX7 |
| ESP32-C3 | `esp32:esp32:esp32c3` | RISC-V |

## 📂 Struktur Proyek
- `MyProject.ino`: File utama logika program.
- `ble_ota.h`: Header untuk fungsi Bluetooth OTA.
- `.github/workflows/`: Folder konfigurasi otomatisasi GitHub.

## 📥 Cara Mendapatkan File .bin
1. Lakukan perubahan pada kode `.ino` atau `.h`.
2. Klik **Commit changes** dan **Push**.
3. Buka tab **Actions** di bagian atas halaman GitHub ini.
4. Klik pada alur kerja (Workflow) terbaru yang sedang berjalan.
5. Setelah selesai (centang hijau), scroll ke bawah ke bagian **Artifacts**.
6. Download file `.bin` sesuai dengan tipe board yang Anda miliki.

## ⚠️ Catatan Penting
- File `.bin` yang dihasilkan dapat di-flash menggunakan aplikasi Android seperti **ESP32 Loader** atau melalui fitur **OTA Update** yang sudah tertanam di kode.
- Untuk penambahan library eksternal, modifikasi file `compile.yml` pada bagian `Install Libraries`.

