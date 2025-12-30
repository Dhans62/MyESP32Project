ESP32 Multi-Platform Build System
Sistem otomasi kompilasi firmware untuk ekosistem ESP32 menggunakan GitHub Actions. Repositori ini dirancang khusus untuk mendukung alur kerja pengembangan perangkat keras (hardware) secara remote, baik melalui PC maupun perangkat mobile (Android).
Arsitektur Board & Versi Core
Pemilihan versi core didasarkan pada stabilitas dan dukungan arsitektur masing-masing chip:
| Board | Arsitektur | Versi Core | Status |
|---|---|---|---|
| ESP32 Classic | Xtensa LX6 | 2.0.17 (LTS) | Stabil / Produksi |
| ESP32-C3 | RISC-V | 3.0.7 | Modern / Terbaru |
Mekanisme Build (Trigger)
Untuk efisiensi sumber daya dan menghindari kompilasi yang tidak perlu, sistem build hanya akan aktif jika:
 * Terjadi perubahan pada file .ino di branch utama.
 * Terjadi perubahan file di dalam direktori run-build/.
 * Dipicu secara manual melalui tab Actions (Workflow Dispatch).
Distribusi Firmware
Setiap proses build yang berhasil akan menghasilkan dua jenis artefak di halaman Releases:
1. OTA Update (.bin)
File binary tunggal yang digunakan untuk pembaruan jarak jauh (Over-The-Air) melalui WiFi atau Bluetooth. Cocok untuk perangkat yang sudah memiliki firmware dasar.
2. Full Flash Bundle (.zip)
Paket lengkap untuk inisialisasi awal perangkat baru atau pemulihan perangkat yang gagal booting. Diperlukan untuk flashing via kabel (USB/OTG). Isi paket:
 * bootloader.bin
 * partitions.bin
 * main.ino.bin (Firmware utama)
Panduan Flashing (Khusus Pengguna Android)
Jika Anda tidak memiliki akses ke PC, gunakan aplikasi flasher di Android (seperti ESP32 Flash/Erase) dengan konfigurasi alamat memori (offset) berikut:
| Nama File | Alamat Memori (Offset) |
|---|---|
| bootloader.bin | 0x0 |
| partitions.bin | 0x8000 |
| *.ino.bin | 0x10000 |
Instruksi Pengembangan
 * Tambah Library: Modifikasi file compaile.yml pada langkah Install Core jika memerlukan library eksternal.
 * Uji Kode: Pastikan nama file .ino sesuai dengan struktur repositori agar kompilasi berjalan lancar.
 * Pemicu Build: Jika hanya mengubah file .h atau dokumentasi, build tidak akan berjalan. Ubah isi folder run-build untuk memaksa proses kompilasi ulang.
