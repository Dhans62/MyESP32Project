# ESP32 Automated Firmware Build

Sistem ini melakukan kompilasi otomatis setiap ada perubahan kode di branch utama. Hasil kompilasi berupa file binary (.bin) yang siap di-flash ke perangkat ESP32.

## Board yang Didukung
- **ESP32 Dev Kit V1**: Core Xtensa LX6
- **ESP32-S3**: Core Xtensa LX7
- **ESP32-C3**: Core RISC-V

## Informasi Teknis
- **Arduino Core**: v3.0.7
- **Metode**: GitHub Actions & GitHub Releases
- **Penamaan File**: `[NamaBoard]-[YYYYMMDD]-[HHMM].bin`

## Cara Mengambil Firmware
1. Buka halaman **Releases**.
2. Cari bagian **Latest Firmware Build**.
3. Di bawah **Assets**, pilih file sesuai board Anda dengan timestamp paling baru (teratas).
4. Download dan gunakan aplikasi flasher (via USB) atau sistem OTA (via BLE/WiFi).

## Pengembangan
Untuk menambahkan library, modifikasi `compile.yml` pada bagian `Setup ESP32 Core`. Selalu pastikan folder proyek memiliki nama yang sama dengan file `.ino` utama.
