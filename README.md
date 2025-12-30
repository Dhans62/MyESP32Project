# ESP32 Multi-Platform Build System

Sistem otomasi kompilasi firmware untuk ekosistem ESP32 menggunakan GitHub Actions. Repositori ini dirancang untuk mendukung alur kerja pengembangan perangkat keras secara remote, terutama bagi pengguna mobile (Android) yang tidak memiliki akses ke PC.

## Arsitektur Board & Versi Core
Pemilihan versi core didasarkan pada stabilitas dan dukungan arsitektur masing-masing chip:

| Board | Arsitektur | Versi Core | Status |
| :--- | :--- | :--- | :--- |
| **ESP32 Classic** | Xtensa LX6 | **2.0.17 (LTS)** | Stabil / Produksi |
| **ESP32-C3** | RISC-V | **3.0.7** | Modern / Terbaru |

## Mekanisme Build (Trigger)
Untuk efisiensi sumber daya, sistem build hanya akan aktif jika:
1. Terjadi perubahan pada file `.ino` di branch utama.
2. Terjadi perubahan file di dalam direktori `run-build/`.
3. Dipicu secara manual melalui tab **Actions** (Workflow Dispatch).

## Distribusi Firmware
Setiap proses build yang berhasil akan menghasilkan dua jenis artefak di halaman **Releases**:

### 1. OTA Update (.bin)
File binary tunggal yang digunakan untuk pembaruan jarak jauh (Over-The-Air) melalui WiFi atau Bluetooth. Cocok untuk perangkat yang sudah memiliki firmware dasar dan skema partisi yang sesuai.

### 2. Full Flash Bundle (.zip)
Paket lengkap untuk inisialisasi awal perangkat baru atau pemulihan perangkat yang gagal booting (brick). Diperlukan untuk flashing via kabel (USB/OTG). Isi paket:
* `bootloader.bin`
* `partitions.bin`
* `[ProjectName].ino.bin` (Firmware utama)

## Panduan Flashing (Khusus Pengguna Android)
Gunakan aplikasi flasher di Android (seperti ESP32 Flash/Erase) dengan konfigurasi alamat memori (offset) berikut:

| Nama File | Alamat Memori (Offset) |
| :--- | :--- |
| `bootloader.bin` | `0x0` |
| `partitions.bin` | `0x8000` |
| `*.ino.bin` | `0x10000` |



## Instruksi Pengembangan
1. **Pemicu Build**: build akan berjalan otomatis jika mengubah bah isi folder `run-build` atau file `.ino` untuk memulai kompilasi.
2. **Tambah Library**: Modifikasi file `compaile.yml` pada bagian `Install Core` jika memerlukan library pihak ketiga.
3. **Validasi**: Selalu pastikan folder proyek memiliki nama yang konsisten dengan file sketsa utama.
