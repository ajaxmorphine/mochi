// Mochi made by: @ajaxmorphine

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include <pitches.h>
#include <WiFi.h>

// ---------------- PIN CONFIGURATION ESP32-C3 SUPER MINI ----------------
#define TOUCH_PIN      6
#define BUZZER_PIN     7    
#define I2C_SDA        8    
#define I2C_SCL        9   

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------------- STRUCT & LIST WI-FI (HARDCODED ONLY) ----------------
struct WifiNetwork {
  const char* ssid;
  const char* password;
};

const WifiNetwork knownNetworks[] = {
  { "Kalorimeter", "Modul_2#" },
  { "ITK-A.X", "K@mpusM3rdeka!" },
  { "ITK-B.X", "K@mpusM3rdeka!" },
  { "ITK-E.X", "K@mpusM3rdeka!" },
  { "ITK-F.X", "K@mpusM3rdeka!" },
  { "ITK-G.X", "K@mpusM3rdeka!" },
};
const int NUM_NETWORKS = sizeof(knownNetworks) / sizeof(knownNetworks[0]);

const long gmtOffset_sec = 28800;  // Diatur ke 28800 untuk WITA (UTC+8)
const int daylightOffset_sec = 0;  // Indonesia tidak memakai DST
const char* ntpServer = "0.id.pool.ntp.org";

// ---------------- Status Koneksi Global ----------------
bool isOfflineMode = false;

// ---------------- Eye geometry ----------------
int leftEyeX = 45;
int rightEyeX = 80;
int eyeY = 18;
int eyeWidth = 25;
int eyeHeight = 30;

int targetOffsetX = 0;
int targetOffsetY = 0;
int moveSpeed = 5;

int blinkState = 0;
int blinkDelay = 4000;
unsigned long lastBlinkTime = 0;
unsigned long moveTime = 0;

static int offsetX = 0;
static int offsetY = 0;

// ---------------- Touch handling (debounce + tiered hold + multi-tap) ----------------
bool touchRaw = false;
bool touchStable = false;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_MS = 25;

unsigned long touchStartTime = 0;
const unsigned long CONFIRM_HOLD_MS = 450;
const unsigned long CANCEL_HOLD_MS = 1200;

bool touchTapEvent = false;
bool touchConfirmEvent = false;
bool touchCancelEvent = false;
bool touchDoubleTapEvent = false;

int tapCount = 0;
unsigned long lastTapTime = 0;
const unsigned long MULTI_TAP_WINDOW = 350;

// ---------------- Menu System Flags ----------------
int currentSelectedMenu = 0;
bool isInMenuSelection = false;
const int TOTAL_MENU = 4;  
// 0: Game (Butter Slide), 1: Jukebox, 2: Wi-Fi Graph, 3: Sys Monitor

// ---------------- Idle Display Variant ----------------
int idleDisplayMode = 0;  // 0: mata normal, 1: matrix, 2: audio visualizer, 3: jam
const int TOTAL_IDLE_DISPLAY_MODES = 4; 

// ---------------- Lock System Flag ----------------
bool isSystemLocked = false; 

// ---------------- Custom Name Tag ----------------
String customNameTag = "";

// ---------------- Matrix Screensaver Variables ----------------
#define MATRIX_COLS 21
int matrixY[MATRIX_COLS];

// ---------------- Wi-Fi Graph Variables ----------------
#define GRAPH_WIDTH 128
#define GRAPH_HEIGHT 32
int rssiHistory[GRAPH_WIDTH];
unsigned long lastGraphUpdateTime = 0;

// ---------------- Audio Visualizer Bar Variables ----------------
#define VISUAL_BAR_COUNT 16 
int visualBarHeights[VISUAL_BAR_COUNT];

// ---------------- Butter Slide Game Variables ----------------
const int BUTTER_GROUND_Y = 54;
float butterY = 44.0;
float butterVelocity = 0.0;
const float BUTTER_GRAVITY = 0.6;
const float BUTTER_JUMP = -7.5;
const int BUTTER_SIZE = 10;

// Logika terbalik-balik (Rotasi Visual Animasi)
float butterAngle = 0.0; 
bool isButterJumping = false;

int obstacleX = 128;
int obstacleWidth = 6;
int obstacleHeight = 10;
int gameSpeed = 4;
int score = 0;
unsigned long lastGameUpdate = 0;

// ---------------- Game state machine ----------------
enum GameMode { MODE_IDLE,
                MODE_BUTTER_PLAY,
                MODE_GAMEOVER,
                MODE_CLOCK,
                MODE_JUKEBOX,
                MODE_MATRIX,
                MODE_WIFIGRAPH,
                MODE_SYSMON };
GameMode gameMode = MODE_IDLE;

// ---------------- Jukebox Custom Song Arrays ----------------
const int SONG_LENGTH = 111;
const int melody[] PROGMEM = {
  NOTE_F4, NOTE_F4, NOTE_D4, NOTE_AS3, NOTE_D4, NOTE_F4, NOTE_AS4, NOTE_D5,
  NOTE_D5, NOTE_C5, NOTE_AS4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_F4, NOTE_F4,
  NOTE_D5, NOTE_D5, NOTE_C5, NOTE_AS4, NOTE_A4, NOTE_G4, NOTE_A4, NOTE_AS4,
  NOTE_AS4, NOTE_F4, NOTE_D4, NOTE_AS3, NOTE_F4, NOTE_F4, NOTE_D4, NOTE_AS3,
  NOTE_D4, NOTE_F4, NOTE_AS4, NOTE_D5, NOTE_D5, NOTE_C5, NOTE_AS4, NOTE_D4,
  NOTE_E4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_D5, NOTE_D5, NOTE_C5, NOTE_AS4,
  NOTE_A4, NOTE_G4, NOTE_A4, NOTE_AS4, NOTE_AS4, NOTE_F4, NOTE_D4, NOTE_AS3,
  NOTE_D5, NOTE_D5, NOTE_D5, NOTE_DS5, NOTE_F5, NOTE_F5, NOTE_DS5, NOTE_D5,
  NOTE_C5, NOTE_D5, NOTE_DS5, NOTE_DS5, NOTE_DS5, NOTE_D5, NOTE_D5, NOTE_C5,
  NOTE_AS4, NOTE_A4, NOTE_G4, NOTE_A4, NOTE_AS4, NOTE_D4, NOTE_E4, NOTE_F4,
  NOTE_F4, NOTE_AS4, NOTE_AS4, NOTE_AS4, NOTE_A4, NOTE_G4, NOTE_G4, NOTE_G4,
  NOTE_C5, NOTE_DS5, NOTE_D5, NOTE_C5, NOTE_AS4, NOTE_AS4, NOTE_A4, NOTE_F4,
  NOTE_F4, NOTE_AS4, NOTE_AS4, NOTE_C5, NOTE_D5, NOTE_DS5, NOTE_F5, NOTE_AS4,
  NOTE_C5, NOTE_D5, NOTE_D5, NOTE_DS5, NOTE_C5, NOTE_AS4, NOTE_AS4
};

const int noteDurations[] PROGMEM = {
  8, 16, 16, 4, 4, 4, 2, 8,
  16, 16, 4, 4, 4, 2, 8, 8,
  4, 8, 8, 4, 2, 8, 8, 4,
  4, 4, 4, 4, 8, 16, 16, 4,
  4, 4, 2, 8, 16, 16, 4, 4,
  4, 2, 8, 8, 4, 8, 8, 4,
  2, 8, 8, 4, 4, 4, 4, 4,
  8, 8, 4, 4, 4, 2, 8, 8,
  4, 4, 4, 2, 4, 4, 8, 8,
  4, 2, 8, 8, 4, 4, 4, 2,
  4, 4, 4, 8, 8, 4, 4, 4,
  4, 8, 8, 8, 8, 4, 2, 8,
  8, 4, 8, 8, 8, 8, 2, 8,
  8, 4, 8, 8, 4, 2, 4
};

int tuneIndex = -1;
unsigned long tuneNextTime = 0;

unsigned long buzzerEndTime = 0;
bool buzzerActive = false;
int currentBaseFreq = 0;
bool applyVibrato = false;

// ---------------- PLATFORMIO FORWARD DECLARATIONS ----------------
void playTone(int freq, int durationMs, bool specialEffect = false);
void updateBuzzer();
void startJukebox();
void updateJukebox();
void readTouch(unsigned long currentTime);
void runIdle(unsigned long currentTime);
void drawMatrixContent(unsigned long currentTime);
void drawAudioVisualizerContent(unsigned long currentTime);
void drawClockContent(unsigned long currentTime);
void runMatrixScreensaver(unsigned long currentTime);
void runWifiGraphMode(unsigned long currentTime);
void runSysMonitorMode(unsigned long currentTime);
void runClockMode(unsigned long currentTime);
void runJukeboxMode(unsigned long currentTime);
void startButterGame();
void runButterGame(unsigned long currentTime);
void startGameOver(unsigned long currentTime);
void runGameOver(unsigned long currentTime);
void drawHint(const char* text);
void drawEye(int eyeX, int eyeY, int w, int h);
void drawEyesNormal(int ox, int oy, bool blinking);
void drawEyesSurprised();
void drawEyesHappy();
void drawEyesHappyAnimation(int dy);
void drawEyesSad();

// ----------------- PLATFORMIO CORE SETUP -----------------
void setup() {
  Wire.begin(I2C_SDA, I2C_SCL);
  pinMode(TOUCH_PIN, INPUT);
  ledcSetup(0, 2000, 8);         // Channel 0, Frekuensi 2000Hz, Resolusi 8-bit
  ledcAttachPin(BUZZER_PIN, 0);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  for (int i = 0; i < MATRIX_COLS; i++) {
    matrixY[i] = random(-40, 0);
  }

  for (int i = 0; i < GRAPH_WIDTH; i++) {
    rssiHistory[i] = -100;
  }

  for (int i = 0; i < VISUAL_BAR_COUNT; i++) {
    visualBarHeights[i] = 0;
  }

  #if defined(ESP32)
    btStop(); 
  #endif

  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(200);

  WiFi.mode(WIFI_STA);
  delay(100);

  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.setTxPower(WIFI_POWER_8_5dBm); // TX Power dikunci di 8.5 dBm
  delay(100);

  bool connected = false;
  int i = 0; 
  
  while (true) {
    WiFi.disconnect();
    delay(100);

    display.clearDisplay();
    display.setCursor(0, 5);
    display.print("Scanning Wi-Fi...");
    display.setCursor(0, 20);
    display.print("Try: "); display.print(knownNetworks[i].ssid);
    display.setCursor(0, 40);
    display.print("[Tap] Skip Net");
    display.setCursor(0, 52);
    display.print("[Hold] Offline Mode");
    display.display();

    WiFi.begin(knownNetworks[i].ssid, knownNetworks[i].password);

    unsigned long startAttempt = millis();
    bool skipRequested = false;
    unsigned long touchStartTimer = 0;
    
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 8000) {
      if (digitalRead(TOUCH_PIN) == HIGH) {
        if (touchStartTimer == 0) {
          touchStartTimer = millis();
        }
        if (millis() - touchStartTimer >= CONFIRM_HOLD_MS) {
          isOfflineMode = true;
          break; 
        }
      } else {
        if (touchStartTimer > 0 && (millis() - touchStartTimer < CONFIRM_HOLD_MS)) {
          skipRequested = true;
          break;
        }
        touchStartTimer = 0;
      }
      delay(30); 
    }

    if (isOfflineMode) {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      break;
    }

    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }

    i = (i + 1) % NUM_NETWORKS;
  }

  display.clearDisplay();
  display.setCursor(0, 15);
  
  if (isOfflineMode) {
    display.setTextSize(1);
    display.print(">> OFFLINE MODE <<");
    display.setCursor(0, 32);
    display.print("System operational.");
    display.display();
  } else if (connected) {
    display.print("Connected to:");
    display.setCursor(0, 27);
    display.print(WiFi.SSID());
    display.setCursor(0, 42);
    display.print("IP: ");
    display.print(WiFi.localIP().toString());
    display.display();

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  }
  
  delay(1500);
  isInMenuSelection = false;
  randomSeed(micros());

  playTone(4000, 100);
}

// ----------------- PLATFORMIO CORE LOOP -----------------
void loop() {
  unsigned long currentTime = millis();

  readTouch(currentTime);
  updateBuzzer();

  if (gameMode == MODE_JUKEBOX) {
    updateJukebox();
  }

  if (gameMode == MODE_IDLE) {
    if (isSystemLocked) {
      if (touchCancelEvent) {
        touchCancelEvent = false;
        isSystemLocked = false;
        playTone(880, 80);
        delay(80);
        playTone(1200, 100);
      }
      touchTapEvent = false;
      touchConfirmEvent = false;
    } else {
      if (!isInMenuSelection && touchConfirmEvent) {
        touchConfirmEvent = false;
        playTone(440, 60);
        delay(60);
        playTone(880, 80);
        isInMenuSelection = true;
      } else if (isInMenuSelection && touchTapEvent) {
        touchTapEvent = false;
        currentSelectedMenu = (currentSelectedMenu + 1) % TOTAL_MENU;
        playTone(500 + (currentSelectedMenu * 100), 50);
      } else if (isInMenuSelection && touchConfirmEvent) {
        touchConfirmEvent = false;

        if (currentSelectedMenu == 0) {
          startButterGame();
        } else if (currentSelectedMenu == 1) { 
          playTone(587, 60);
          delay(70);
          playTone(659, 60);
          startJukebox();
          gameMode = MODE_JUKEBOX;
        } else if (currentSelectedMenu == 2) { 
          playTone(600, 80);
          gameMode = MODE_WIFIGRAPH;
        } else if (currentSelectedMenu == 3) { 
          playTone(800, 80);
          gameMode = MODE_SYSMON;
        }
      } else if (isInMenuSelection && touchCancelEvent) {
        touchCancelEvent = false;
        playTone(300, 100);
        isInMenuSelection = false;
      } else if (!isInMenuSelection && touchCancelEvent) {
        touchCancelEvent = false;
        isSystemLocked = true;
        isInMenuSelection = false;
        playTone(300, 100);
        delay(100);
        playTone(150, 150);
      } else if (!isInMenuSelection && touchTapEvent) {
        touchTapEvent = false;
        idleDisplayMode = (idleDisplayMode + 1) % TOTAL_IDLE_DISPLAY_MODES;
        playTone(1000 + (idleDisplayMode * 200), 60);
      }
    }
  }

  switch (gameMode) {
    case MODE_IDLE: runIdle(currentTime); break;
    case MODE_BUTTER_PLAY: runButterGame(currentTime); break;
    case MODE_GAMEOVER: runGameOver(currentTime); break;
    case MODE_CLOCK: runClockMode(currentTime); break;
    case MODE_JUKEBOX: runJukeboxMode(currentTime); break;
    case MODE_MATRIX: runMatrixScreensaver(currentTime); break;
    case MODE_WIFIGRAPH: runWifiGraphMode(currentTime); break;
    case MODE_SYSMON: runSysMonitorMode(currentTime); break;
  }

  delay(20);
}

// ----------------- CORE UTILITY FUNCTIONS -----------------
void playTone(int freq, int durationMs, bool specialEffect) {
  currentBaseFreq = freq;
  applyVibrato = specialEffect;
  
  if (freq == 0) {
    ledcWrite(0, 0); 
  } else {
    ledcWriteTone(0, freq);
    ledcWrite(0, 80); 
  }
  
  buzzerEndTime = millis() + durationMs;
  buzzerActive = true;
}

void updateBuzzer() {
  if (buzzerActive) {
    if (millis() >= buzzerEndTime) {
      ledcWrite(0, 0); 
      buzzerActive = false;
    } else if (applyVibrato && currentBaseFreq > 0) {
      float angle = (millis() % 150) * (2.0 * 3.14159 / 150.0);
      int smoothPitchShift = sin(angle) * 3; 
      
      ledcWriteTone(0, currentBaseFreq + smoothPitchShift);
      ledcWrite(0, 80); 
    }
  }
}

void startJukebox() {
  tuneIndex = 0;
  tuneNextTime = millis();
}

void updateJukebox() {
  if (tuneIndex < 0 || tuneIndex >= SONG_LENGTH) {
    if (tuneIndex >= SONG_LENGTH) {
      tuneIndex = -1;
    }
    return;
  }

  if (millis() >= tuneNextTime) {
    int currentNote = pgm_read_word_near(melody + tuneIndex);
    int durationType = pgm_read_word_near(noteDurations + tuneIndex);
    int noteDuration = 2000 / durationType;

    playTone(currentNote, noteDuration, true);
    tuneNextTime = millis() + (noteDuration * 1.30);
    tuneIndex++;
  }
}

void readTouch(unsigned long currentTime) {
  touchTapEvent = false;
  touchConfirmEvent = false;
  touchCancelEvent = false;
  touchDoubleTapEvent = false;

  bool reading = digitalRead(TOUCH_PIN);

  if (reading != touchRaw) {
    lastDebounceTime = currentTime;
    touchRaw = reading;
  }

  if (currentTime - lastDebounceTime > DEBOUNCE_MS) {
    if (touchStable != touchRaw) {
      touchStable = touchRaw;

      if (touchStable) {
        touchStartTime = currentTime;
      } else {
        unsigned long heldDuration = currentTime - touchStartTime;

        if (heldDuration >= CANCEL_HOLD_MS) {
          touchCancelEvent = true;
          tapCount = 0;
        } else if (heldDuration >= CONFIRM_HOLD_MS) {
          touchConfirmEvent = true;
          tapCount = 0;
        } else {
          if (currentTime - lastTapTime < MULTI_TAP_WINDOW) {
            tapCount++;
          } else {
            tapCount = 1;
          }
          lastTapTime = currentTime;

          if (tapCount == 1) {
            touchTapEvent = true;
          } else if (tapCount == 2) {
            touchDoubleTapEvent = true;
            touchTapEvent = false;
            tapCount = 0;
          }
        }
      }
    }
  }

  if (tapCount > 0 && (currentTime - lastTapTime > MULTI_TAP_WINDOW)) {
    tapCount = 0;
  }
}

// ----------------- RUN SCENARIOS -----------------
void runIdle(unsigned long currentTime) {
  if (currentTime - lastBlinkTime > blinkDelay && blinkState == 0) {
    blinkState = 1;
    lastBlinkTime = currentTime;
  } else if (currentTime - lastBlinkTime > 150 && blinkState == 1) {
    blinkState = 0;
    lastBlinkTime = currentTime;
  }

  display.clearDisplay();

  if (isSystemLocked) {
    drawEyesNormal(0, 0, true);
    display.drawCircle(63, 49, 4, WHITE);
    display.fillRect(59, 49, 9, 6, BLACK); 
    display.fillRoundRect(57, 52, 13, 10, 2, WHITE);
    display.fillRect(63, 55, 2, 4, BLACK);
  
  } else if (!isInMenuSelection) {
    if (idleDisplayMode == 0) {
      if (currentTime - moveTime > random(1500, 3000) && blinkState == 0) {
        int movementType = random(0, 8);
        if (movementType == 0) {
          targetOffsetX = -10;
          targetOffsetY = 0;
        } else if (movementType == 1) {
          targetOffsetX = 10;
          targetOffsetY = 0;
        } else if (movementType == 2) {
          targetOffsetX = -10;
          targetOffsetY = -8;
        } else if (movementType == 3) {
          targetOffsetX = 10;
          targetOffsetY = -8;
        } else {
          targetOffsetX = 0;
          targetOffsetY = 0;
        }
        moveTime = currentTime;
      }
      offsetX += (targetOffsetX - offsetX) / moveSpeed;
      offsetY += (targetOffsetY - offsetY) / moveSpeed;

      drawEyesNormal(offsetX, offsetY, blinkState == 1);
      drawHint(customNameTag.c_str());
    } else if (idleDisplayMode == 3) {
      drawMatrixContent(currentTime);
    } else if (idleDisplayMode == 2) {
      drawAudioVisualizerContent(currentTime);
    } else if (idleDisplayMode == 1) {
      drawClockContent(currentTime);
    }
  } else {
    // ------------------- MENU SELECTION GRAPHICS -------------------
    int iconCenterX = 64;
    int iconCenterY = 27;

    if (currentSelectedMenu == 0) {
      // --- IKON BUTTER SLIDE ---
      display.drawFastHLine(iconCenterX - 16, iconCenterY + 8, 32, WHITE); // Lantai dasar
      display.drawRoundRect(iconCenterX - 6, iconCenterY - 4, 12, 10, 2, WHITE); // Mentega miring dikit
      display.fillRect(iconCenterX + 8, iconCenterY + 2, 4, 6, WHITE); // Rintangan kecil
      drawHint("[ BUTTER SLIDE ]");
      
    } else if (currentSelectedMenu == 1) { 
      // --- IKON JUKEBOX MUSIC (Not Balok) ---
      display.fillRect(iconCenterX - 6, iconCenterY - 10, 4, 16, WHITE);  // Tiang kiri
      display.fillRect(iconCenterX + 6, iconCenterY - 10, 4, 16, WHITE);  // Tiang kanan
      display.fillRoundRect(iconCenterX - 12, iconCenterY + 2, 8, 6, 2, WHITE); // Bulatan not kiri
      display.fillRoundRect(iconCenterX, iconCenterY + 2, 8, 6, 2, WHITE);  // Bulatan not kanan
      display.fillTriangle(iconCenterX - 6, iconCenterY - 10, iconCenterX + 10, iconCenterY - 14, iconCenterX + 10, iconCenterY - 10, WHITE); // Bendera penghubung atas
      display.fillRect(iconCenterX - 6, iconCenterY - 10, 16, 4, WHITE);
      drawHint("[ JUKEBOX MUSIC ]");
      
    } else if (currentSelectedMenu == 2) { 
      // --- IKON WI-FI GRAPH (Sinyal Wave Berundak) ---
      display.fillRect(iconCenterX - 15, iconCenterY + 6, 5, 4, WHITE);   // Bar 1 (Paling rendah)
      display.fillRect(iconCenterX - 7, iconCenterY + 2, 5, 8, WHITE);    // Bar 2
      display.fillRect(iconCenterX + 1, iconCenterY - 3, 5, 13, WHITE);   // Bar 3
      display.fillRect(iconCenterX + 9, iconCenterY - 9, 5, 19, WHITE);   // Bar 4 (Paling tinggi)
      display.drawLine(iconCenterX - 13, iconCenterY + 6, iconCenterX - 5, iconCenterY + 2, WHITE);
      display.drawLine(iconCenterX - 5, iconCenterY + 2, iconCenterX + 3, iconCenterY - 3, WHITE);
      display.drawLine(iconCenterX + 3, iconCenterY - 3, iconCenterX + 11, iconCenterY - 9, WHITE);
      drawHint("[ WI-FI GRAPH ]");
      
    } else if (currentSelectedMenu == 3) { 
      // --- IKON SYSTEM MONITOR (Roda Gerigi / Cogwheel) ---
      display.drawCircle(iconCenterX, iconCenterY, 9, WHITE);       // Struktur lingkaran luar
      display.drawCircle(iconCenterX, iconCenterY, 4, WHITE);       // Lingkaran poros dalam
      display.fillRect(iconCenterX - 2, iconCenterY - 13, 5, 4, WHITE);  // Atas
      display.fillRect(iconCenterX - 2, iconCenterY + 10, 5, 4, WHITE);  // Bawah
      display.fillRect(iconCenterX - 13, iconCenterY - 2, 4, 5, WHITE);  // Kiri
      display.fillRect(iconCenterX + 10, iconCenterY - 2, 4, 5, WHITE);  // Kanan
      display.fillRect(iconCenterX - 9, iconCenterY - 9, 3, 3, WHITE);
      display.fillRect(iconCenterX + 7, iconCenterY + 6, 3, 3, WHITE);
      display.fillRect(iconCenterX + 7, iconCenterY - 9, 3, 3, WHITE);
      display.fillRect(iconCenterX - 9, iconCenterY + 6, 3, 3, WHITE);
      drawHint("[ SYS MONITOR ]");
    }

    // Navigasi Index Menu bawah kanan
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(110, 56);
    display.print(currentSelectedMenu + 1);
    display.print("/");
    display.print(TOTAL_MENU);
  }

  display.display();
}

void drawMatrixContent(unsigned long currentTime) {
  display.setTextSize(1);
  display.setTextColor(WHITE);

  for (int i = 0; i < MATRIX_COLS; i++) {
    int x = i * 6;
    display.setCursor(x, matrixY[i]);
    display.print(random(0, 2));

    matrixY[i] += 4;
    if (matrixY[i] > SCREEN_HEIGHT) {
      matrixY[i] = random(-20, 0);
    }
  }
}

void drawAudioVisualizerContent(unsigned long currentTime) {
  int maxTargetHeight = (gameMode == MODE_JUKEBOX) ? SCREEN_HEIGHT - 6 : 35;

  for (int i = 0; i < VISUAL_BAR_COUNT; i++) {
    int target = random(2, maxTargetHeight);
    
    if (visualBarHeights[i] < target) {
      visualBarHeights[i] += 5;
    } else {
      visualBarHeights[i] -= 3;
    }
    
    visualBarHeights[i] = constrain(visualBarHeights[i], 0, SCREEN_HEIGHT - 2);
    
    int xPos = i * 8; 
    int barWidth = 6; 
    
    display.fillRect(xPos, SCREEN_HEIGHT - visualBarHeights[i], barWidth, visualBarHeights[i], WHITE);
  }
}

void drawClockContent(unsigned long currentTime) {
  display.fillRect(leftEyeX, eyeY + 10, eyeWidth, 3, WHITE);
  display.fillRect(rightEyeX, eyeY + 10, eyeWidth, 3, WHITE);

  int zOffset = (millis() / 500) % 3;
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(110 - zOffset * 2, 10 - zOffset * 2);
  display.print("z");
  display.setCursor(116 - zOffset * 2, 4 - zOffset * 2);
  display.print("Z");

  struct tm timeinfo;
  bool timeValid = false;
  if (!isOfflineMode) {
    timeValid = getLocalTime(&timeinfo, 0); 
  }

  display.setTextSize(2);
  display.setCursor(16, 40);

  if (timeValid && !isOfflineMode) {
    if (timeinfo.tm_hour < 10) display.print("0");
    display.print(timeinfo.tm_hour);
    display.print((millis() % 1000 < 500) ? ":" : " ");
    if (timeinfo.tm_min < 10) display.print("0");
    display.print(timeinfo.tm_min);

    display.setTextSize(1);
    display.setCursor(80, 40);
    if (timeinfo.tm_sec < 10) display.print("0");
    display.print(timeinfo.tm_sec);
  } else {
    display.print("OFFLINE");
  }
}

void runMatrixScreensaver(unsigned long currentTime) {
  drawMatrixContent(currentTime);
  display.display();

  if (touchTapEvent) {
    touchTapEvent = false;
    playTone(1200, 60);
    gameMode = MODE_IDLE;
  }
}

void runWifiGraphMode(unsigned long currentTime) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  if (isOfflineMode) {
    display.setCursor(15, 25);
    display.print("Wi-Fi is Disabled");
    display.setCursor(16, 56);
    display.print("[tap once to exit]");
    display.display();
    if (touchTapEvent) {
      touchTapEvent = false;
      playTone(500, 80);
      gameMode = MODE_IDLE;
    }
    return;
  }

  if (currentTime - lastGraphUpdateTime >= 100) {
    lastGraphUpdateTime = currentTime;
    int rssi = WiFi.RSSI();

    for (int i = 0; i < GRAPH_WIDTH - 1; i++) {
      rssiHistory[i] = rssiHistory[i + 1];
    }
    rssiHistory[GRAPH_WIDTH - 1] = rssi;
  }

  int currentRSSI = WiFi.RSSI();
  display.setCursor(0, 0);
  display.print("SSID: ");
  display.print(WiFi.SSID());
  display.setCursor(0, 10);
  display.print("RSSI: ");
  display.print(currentRSSI);
  display.print(" dBm");

  display.drawRect(0, 22, 128, 32, WHITE);

  for (int x = 0; x < GRAPH_WIDTH; x++) {
    int yVal = map(rssiHistory[x], -100, -30, 0, 30);
    yVal = constrain(yVal, 0, 30);
    if (rssiHistory[x] != -100) {
      display.drawFastVLine(x, 53 - yVal, yVal, WHITE);
    }
  }

  display.setCursor(16, 56);
  display.print("[tap once to exit]");
  display.display();

  if (touchTapEvent) {
    touchTapEvent = false;
    playTone(500, 80);
    gameMode = MODE_IDLE;
  }
}

void runSysMonitorMode(unsigned long currentTime) {
  display.clearDisplay();
  display.drawRect(0, 0, 128, 54, WHITE);
  display.drawRect(2, 2, 124, 50, WHITE);

  display.setTextSize(1);
  display.setTextColor(WHITE);

  uint32_t freeHeap = ESP.getFreeHeap() / 1024;
  uint32_t uptimeS = millis() / 1000;
  float internalTemp = temperatureRead();

  display.setCursor(8, 10);
  display.print("FREE HEAP : ");
  display.print(freeHeap);
  display.print(" KB");
  
  display.setCursor(8, 22);
  display.print("UPTIME    : ");
  display.print(uptimeS);
  display.print(" SEC");

  display.setCursor(8, 34);
  display.print("CPU TEMP  : ");
  if (isnan(internalTemp) || internalTemp < -40.0 || internalTemp > 150.0) {
    display.print("ERR");
  } else {
    display.print(internalTemp, 1);
    display.print(" C");
  }

  display.setCursor(16, 56);
  display.print("[tap once to exit]");
  display.display();

  if (touchTapEvent) {
    touchTapEvent = false;
    playTone(500, 80);
    gameMode = MODE_IDLE;
  }
}

void runClockMode(unsigned long currentTime) {
  display.clearDisplay();
  drawClockContent(currentTime);
  display.display();

  if (touchTapEvent) {
    touchTapEvent = false;
    playTone(500, 80);
    gameMode = MODE_IDLE;
  }
}

void runJukeboxMode(unsigned long currentTime) {
  display.clearDisplay();

  int danceY = (millis() % 400 < 200) ? 2 : -2;
  drawEyesHappyAnimation(danceY);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(22, 2);
  display.print("= USA ANTHEM =");

  display.setCursor(16, 48);
  bool stillPlaying = (tuneIndex >= 0) &&
                       (tuneIndex < SONG_LENGTH || millis() < tuneNextTime);
  if (stillPlaying) {
    display.print("Playing Music...");
  } else {
    ledcWrite(0, 0);
    tuneIndex = -1;
    gameMode = MODE_IDLE;
    return;
  }

  display.setCursor(16, 56);
  display.print("[tap to exit]");
  display.display();

  if (touchTapEvent) {
    touchTapEvent = false;
    ledcWrite(0, 0);
    tuneIndex = -1;
    playTone(500, 80);
    gameMode = MODE_IDLE;
  }
}

// ----------------- BUTTER SLIDE ENGINE -----------------
void startButterGame() {
  score = 0;
  butterY = BUTTER_GROUND_Y - BUTTER_SIZE;
  butterVelocity = 0.0;
  butterAngle = 0.0;
  isButterJumping = false;
  
  obstacleX = 128;
  obstacleHeight = random(8, 15);
  gameSpeed = 4;
  gameMode = MODE_BUTTER_PLAY;
  lastGameUpdate = millis();
  playTone(880, 100);
}

void runButterGame(unsigned long currentTime) {
  if (touchTapEvent) {
    touchTapEvent = false;
    if (!isButterJumping) {
      butterVelocity = BUTTER_JUMP;
      isButterJumping = true;
      playTone(600, 50);
    }
  }

  if (currentTime - lastGameUpdate >= 25) { // Sedikit dipercepat respon framerate-nya
    lastGameUpdate = currentTime;
    
    // Fisika Kubus Mentega
    if (isButterJumping) {
      butterVelocity += BUTTER_GRAVITY;
      butterY += butterVelocity;
      
      // Kecepatan putaran sudut saat melompat (efek terbalik-balik)
      butterAngle += 9.0; 
      if (butterAngle >= 360.0) butterAngle -= 360.0;

      // Cek landing
      if (butterY >= BUTTER_GROUND_Y - BUTTER_SIZE) {
        butterY = BUTTER_GROUND_Y - BUTTER_SIZE;
        isButterJumping = false;
        butterVelocity = 0.0;
        butterAngle = 0.0; // Reset posisi tegak saat di tanah
        playTone(400, 30); // Efek suara mendarat empuk
      }
    }

    // Pergerakan Rintangan
    obstacleX -= gameSpeed;
    if (obstacleX < -obstacleWidth) {
      obstacleX = 128;
      obstacleHeight = random(8, 16);
      score++;
      if (score % 5 == 0 && gameSpeed < 8) {
        gameSpeed++;
      }
    }

    // Deteksi Hitbox Sederhana (AABB)
    int butterLeft = 20;
    int butterRight = 20 + BUTTER_SIZE;
    int butterTop = (int)butterY;
    int butterBottom = (int)butterY + BUTTER_SIZE;

    int obsLeft = obstacleX;
    int obsRight = obstacleX + obstacleWidth;
    int obsTop = BUTTER_GROUND_Y - obstacleHeight;
    int obsBottom = BUTTER_GROUND_Y;

    if (butterRight >= obsLeft && butterLeft <= obsRight && butterBottom >= obsTop && butterTop <= obsBottom) {
      startGameOver(currentTime);
      return;
    }
  }

  display.clearDisplay();
  
  // Garis Tanah
  display.drawFastHLine(0, BUTTER_GROUND_Y, 128, WHITE);
  
  // Gambar Rintangan (Segitiga/Balok Tajam)
  display.fillRect(obstacleX, BUTTER_GROUND_Y - obstacleHeight, obstacleWidth, obstacleHeight, WHITE);

  // Render Visual Mentega dengan Efek Rotasi / Terbalik
  int renderX = 20;
  int renderY = (int)butterY;
  
  if (isButterJumping) {
    // Membuat proyeksi rotasi 90-derajat palsu agar memori hemat (tanpa fungsi sin/cos berat)
    int state = ((int)butterAngle / 45) % 4;
    if (state == 1 || state == 3) {
      // Bentuk berlian/ketupat saat berputar diagonal
      display.fillTriangle(renderX + BUTTER_SIZE/2, renderY - 2, renderX - 2, renderY + BUTTER_SIZE/2, renderX + BUTTER_SIZE + 2, renderY + BUTTER_SIZE/2, WHITE);
      display.fillTriangle(renderX - 2, renderY + BUTTER_SIZE/2, renderX + BUTTER_SIZE + 2, renderY + BUTTER_SIZE/2, renderX + BUTTER_SIZE/2, renderY + BUTTER_SIZE + 2, WHITE);
    } else {
      // Bentuk terbalik penuh atau menyamping
      display.fillRect(renderX, renderY, BUTTER_SIZE, BUTTER_SIZE, WHITE);
      display.drawRect(renderX + 2, renderY + 2, BUTTER_SIZE - 4, BUTTER_SIZE - 4, BLACK); // Garis corak dalam
    }
  } else {
    // Efek Squash (Penyet mentega halus di tanah)
    display.fillRoundRect(renderX, renderY + 1, BUTTER_SIZE, BUTTER_SIZE - 1, 2, WHITE);
  }

  // Papan Skor / XP
  display.setTextSize(1);
  display.setCursor(95, 2);
  display.print("XP:"); display.print(score);
  display.display();
}

void startGameOver(unsigned long currentTime) {
  gameMode = MODE_GAMEOVER;
  ledcWrite(0, 0);
  playTone(300, 200);
  delay(150);
  playTone(150, 400);
}

void runGameOver(unsigned long currentTime) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(12, 12);
  display.print("GAME OVER");
  
  display.setTextSize(1);
  display.setCursor(32, 36);
  display.print("Score: ");
  display.print(score);
  
  display.setCursor(16, 54);
  display.print("[tap to return]");
  display.display();

  if (touchTapEvent) {
    touchTapEvent = false;
    playTone(600, 80);
    gameMode = MODE_IDLE;
  }
}

// ----------------- PURE GRAPHIC RENDERERS -----------------
void drawHint(const char* text) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(2, 56);
  display.print(text);
}

void drawEye(int eyeX, int eyeY, int w, int h) {
  display.fillRoundRect(eyeX, eyeY, w, h, 5, WHITE);
}

void drawEyesNormal(int ox, int oy, bool blinking) {
  if (!blinking) {
    drawEye(leftEyeX + ox, eyeY + oy, eyeWidth, eyeHeight);
    drawEye(rightEyeX + ox, eyeY + oy, eyeWidth, eyeHeight);
  } else {
    display.fillRect(leftEyeX + ox, eyeY + oy + eyeHeight / 2 - 2, eyeWidth, 4, WHITE);
    display.fillRect(rightEyeX + ox, eyeY + oy + eyeHeight / 2 - 2, eyeWidth, 4, WHITE);
  }
}

void drawEyesSurprised() {
  int biggerH = eyeHeight + 14;
  int biggerY = eyeY - 7;
  drawEye(leftEyeX, biggerY, eyeWidth, biggerH);
  drawEye(rightEyeX, biggerY, eyeWidth, biggerH);
}

void drawEyesHappy() {
  int h = 8;
  display.fillRoundRect(leftEyeX, eyeY + 6, eyeWidth, h, 4, WHITE);
  display.fillRoundRect(rightEyeX, eyeY + 6, eyeWidth, h, 4, WHITE);
}

void drawEyesHappyAnimation(int dy) {
  int h = 8;
  display.fillRoundRect(leftEyeX, eyeY + 6 + dy, eyeWidth, h, 4, WHITE);
  display.fillRoundRect(rightEyeX, eyeY + 6 + dy, eyeWidth, h, 4, WHITE);
}

void drawEyesSad() {
  int h = 6;
  display.fillRoundRect(leftEyeX, eyeY + eyeHeight - h, eyeWidth, h, 3, WHITE);
  display.fillRoundRect(rightEyeX, eyeY + eyeHeight - h, eyeWidth, h, 3, WHITE);
}