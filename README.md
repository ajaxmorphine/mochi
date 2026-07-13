🤖 mochi — Smart Desktop Cyber-Pet
mochi adalah sebuah proyek perangkat keras berbasis open-source yang menggabungkan konsep virtual pet interaktif dengan utilitas terminal portabel. Ditenagai oleh microcontroller berperforma tinggi ESP32-C3 Super Mini, mochi menghadirkan kepribadian digital yang hidup melalui ekspresi mata animasi pada layar OLED, sekaligus berfungsi sebagai alat pemantau sistem esp32 (system monitor) dan jaringan lokal.

Proyek ini dibangun menggunakan arsitektur State Machine yang efisien, memastikan transisi yang mulus antara mode ekspresi idle, game konsentrasi, pemutar musik retro, hingga pemindaian sinyal Wi-Fi secara real-time.

🚀 Fitur Utama (Key Features)
1. Animasi Wajah & Ekspresi Interaktif
Dynamic Eye Geometry: Mata mochi dapat berkedip secara acak, melirik ke berbagai arah, dan merespons interaksi pengguna.

Emotional Feedback: Wajah mochi dapat berubah secara instan menjadi ekspresi Normal, Terkejut (Surprised), Senang (Happy), Sedih (Sad), hingga mode Tidur (Sleeping).

Security Lock System: Dilengkapi dengan fitur penguncian sistem terintegrasi yang menampilkan visual gembok unik saat diaktifkan.

2. Tiered Touch Handling (Sistem Navigasi Cerdas)
Navigasi perangkat sepenuhnya dioperasikan melalui satu pin sentuh (capacitive touch pin) dengan pembacaan logika berlapis yang anti-debounce:

Single Tap: Berpindah menu / mengubah mode screensaver saat idle.

Double Tap: Mode interaksi cepat.

Confirm Hold (~450ms): Masuk ke menu pilihan / konfirmasi opsi.

Cancel Hold (~1200ms): Keluar dari menu / mengaktifkan sistem kunci.

3. Sistem Operasi Multi-Mode
🎮 Reaction Game: Game uji ketangkasan refleks pengguna yang dilengkapi dengan perhitungan skor (Scoreing), sistem nyawa (Lives), serta tingkat kesulitan waktu respons yang semakin cepat secara dinamis.

🎵 Chiptune Jukebox: Pemutar musik retro 8-bit (Soviet Anthem) yang memanfaatkan modulasi Hardware PWM (LEDC) terintegrasi dengan osilator getaran halus (Sinusoidal LFO) untuk menghasilkan audio musik chiptune yang jernih.

📶 Wi-Fi RSSI Graph: Memindai dan memvisualisasikan grafik kekuatan sinyal Wi-Fi di sekitar secara real-time ke dalam bentuk grafik bar di layar OLED.

📊 System Monitor (SysMon): Memantau metrik internal perangkat keras secara langsung, meliputi sisa memori RAM (Free Heap Memory dalam KB), durasi aktif (Uptime dalam detik), serta temperatur internal Core CPU.

🔦 Stealth Light Mode: Mengubah fungsi lampu latar LED internal menjadi senter konstan (Steady) maupun penanda darurat (Strobe Mode).

4. Konektivitas & Fleksibilitas Jaringan
Online Mode: Sinkronisasi waktu secara presisi dengan server jam dunia memanfaatkan protokol NTP (Network Time Protocol) melalui Wi-Fi untuk menampilkan jam digital yang akurat saat screensaver.

Offline Mode: Sistem proteksi kegagalan koneksi. Pengguna dapat menahan tombol (Hold) saat pemindaian Wi-Fi awal untuk memaksa perangkat masuk ke mode Offline, membuat seluruh fungsi utilitas non-jaringan tetap beroperasi secara mandiri.

Modular Screensaver: Pilihan tampilan idle yang bervariasi dari animasi mata, Matrix digital rain code, Audio Visualizer grafis, hingga Jam Digital.

🛠️ Spesifikasi Perangkat Keras (Hardware Stack)
Controller: ESP32-C3 Super Mini (RISC-V Architecture)

Display: SSD1306 OLED Display (128x64 Pixels, I2C Interface)

Input: Capacitive Touch Sensor (Pin 6)

Audio: Passive Piezo Buzzer (Digerakkan melalui Jalur Hardware PWM LEDC - Channel 0)

Indicator Light: Blue LED SMD (Pin 20)

💻 Kerangka Perangkat Lunak (Software Stack)
Framework: Arduino Framework (PlatformIO IDE / VS Code)

Libraries:

Wire.h (Komunikasi I2C)

Adafruit_GFX.h & Adafruit_SSD1306.h (Pemrosesan Grafis Layar)

WiFi.h & time.h (Manajemen Jaringan & Waktu NTP)

👨‍💻 Kontributor
Lead Developer / Maker: @ajaxmorphine
