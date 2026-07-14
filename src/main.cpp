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
#define BLUE_LED_PIN  20    

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
const int TOTAL_MENU = 5;  
// 0: Game, 1: Jukebox, 2: Wi-Fi Graph, 3: Sys Monitor, 4: Stealth Light

// ---------------- Idle Display Variant ----------------
int idleDisplayMode = 0;  // 0: mata normal, 1: matrix, 2: audio visualizer, 3: jam
const int TOTAL_IDLE_DISPLAY_MODES = 4; 

// ---------------- Stealth Light Flag ----------------
bool blueLedState = false;   
bool ledStrobeMode = false;  
unsigned long lastStrobeTime = 0;
const unsigned long STROBE_INTERVAL = 100;  

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

// ---------------- Game state machine ----------------
enum GameMode { MODE_IDLE,
                MODE_ARMED,
                MODE_ALERT,
                MODE_HIT,
                MODE_MISS,
                MODE_GAMEOVER,
                MODE_CLOCK,
                MODE_JUKEBOX,
                MODE_MATRIX,
                MODE_WIFIGRAPH,
                MODE_SYSMON,
                MODE_STEALTH_LIGHT };
GameMode gameMode = MODE_IDLE;

int score = 0;
int lives = 3;
unsigned long currentWaitTime = 2000;
const unsigned long MIN_WAIT_TIME = 700;
const unsigned long REACTION_WINDOW = 650;

unsigned long armedStartTime = 0;
unsigned long armedTargetWait = 0;
unsigned long alertStartTime = 0;
unsigned long feedbackShownAt = 0;
const unsigned long FEEDBACK_DURATION = 350;

// ---------------- Jukebox Custom Song Arrays ----------------
const int SONG_LENGTH = 102;
const int melody[] PROGMEM = {
  NOTE_C5, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_E4, NOTE_E4,
  NOTE_A4, NOTE_G4, NOTE_F4, NOTE_G4, NOTE_C4, NOTE_C4, NOTE_D4, NOTE_D4,
  NOTE_E4, NOTE_F4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_D5,
  NOTE_G4, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_D5, NOTE_B4, NOTE_G4, NOTE_C5,
  NOTE_B4, NOTE_A4, NOTE_B4, NOTE_E4, NOTE_E4, NOTE_A4, NOTE_G4, NOTE_F4,
  NOTE_G4, NOTE_C4, NOTE_C4, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_E5,
  NOTE_D5, NOTE_C5, NOTE_B4, NOTE_C5, NOTE_D5, NOTE_G4, NOTE_G4, NOTE_C5,
  NOTE_B4, NOTE_A4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_E4, NOTE_E4, NOTE_C5,
  NOTE_A4, NOTE_B4, NOTE_C5, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_A4, NOTE_C5,
  NOTE_F5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_C5,
  NOTE_C5, NOTE_D5, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_A4,
  NOTE_A4, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_C4, NOTE_C4, NOTE_G4,
  NOTE_A4, NOTE_B4, NOTE_C5,
  NOTE_C5, NOTE_C5, NOTE_C5
};

const int noteDurations[] PROGMEM = {
  1, 8, 4, 6, 16, 4, 8, 8,
  4, 6, 16, 4, 8, 8, 4, 8,
  8, 4, 8, 8, 4, 8, 8, 2,
  8, 4, 6, 16, 4, 8, 8, 4,
  6, 16, 4, 8, 8, 4, 8, 8,
  4, 8, 8, 4, 6, 16, 2, 2,
  8, 8, 8, 8, 3, 8, 2, 2,
  8, 8, 8, 8, 3, 8, 2, 4,
  6, 16, 4, 6, 16, 4, 8, 8,
  2, 2, 8, 8, 8, 8, 3, 8,
  2, 2, 8, 8, 8, 8, 3, 8,
  2, 4, 8, 8, 4, 6, 16, 2,
  4, 4, 1,
  0, 0, 0
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
void armToNextRound(unsigned long currentTime);
void runIdle(unsigned long currentTime);
void drawMatrixContent(unsigned long currentTime);
void drawAudioVisualizerContent(unsigned long currentTime);
void drawClockContent(unsigned long currentTime);
void runStealthLightMode(unsigned long currentTime);
void runMatrixScreensaver(unsigned long currentTime);
void runWifiGraphMode(unsigned long currentTime);
void runSysMonitorMode(unsigned long currentTime);
void runClockMode(unsigned long currentTime);
void runJukeboxMode(unsigned long currentTime);
void runArmed(unsigned long currentTime);
void runAlert(unsigned long currentTime);
void triggerHit(unsigned long currentTime);
void triggerMiss(unsigned long currentTime);
void runFeedback(unsigned long currentTime, bool isHit);
void startGameOver(unsigned long currentTime);
void runGameOver(unsigned long currentTime);
void drawHud();
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
  pinMode(BLUE_LED_PIN, OUTPUT);
  digitalWrite(BLUE_LED_PIN, LOW);

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

  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(200);

  WiFi.mode(WIFI_STA);
  delay(100);

  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
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

  if (blueLedState && ledStrobeMode) {
    if (currentTime - lastStrobeTime >= STROBE_INTERVAL) {
      lastStrobeTime = currentTime;
      digitalWrite(BLUE_LED_PIN, !digitalRead(BLUE_LED_PIN));
    }
  }

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
          score = 0;
          lives = 3;
          currentWaitTime = 2000;
          playTone(600, 80);
          delay(40);
          playTone(900, 100);
          armToNextRound(currentTime);
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
        } else if (currentSelectedMenu == 4) { 
          playTone(900, 80);
          blueLedState = true;
          ledStrobeMode = false;
          digitalWrite(BLUE_LED_PIN, HIGH);
          gameMode = MODE_STEALTH_LIGHT;
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
        // Siklus Tap Idle Berurutan: Mata -> Matrix -> Visualizer -> Jam
        idleDisplayMode = (idleDisplayMode + 1) % TOTAL_IDLE_DISPLAY_MODES;
        playTone(1000 + (idleDisplayMode * 200), 60);
      }
    }
  }

  switch (gameMode) {
    case MODE_IDLE: runIdle(currentTime); break;
    case MODE_ARMED: runArmed(currentTime); break;
    case MODE_ALERT: runAlert(currentTime); break;
    case MODE_HIT: runFeedback(currentTime, true); break;
    case MODE_MISS: runFeedback(currentTime, false); break;
    case MODE_GAMEOVER: runGameOver(currentTime); break;
    case MODE_CLOCK: runClockMode(currentTime); break;
    case MODE_JUKEBOX: runJukeboxMode(currentTime); break;
    case MODE_MATRIX: runMatrixScreensaver(currentTime); break;
    case MODE_WIFIGRAPH: runWifiGraphMode(currentTime); break;
    case MODE_SYSMON: runSysMonitorMode(currentTime); break;
    case MODE_STEALTH_LIGHT: runStealthLightMode(currentTime); break;
  }

  delay(20);
}

// ----------------- CORE UTILITY FUNCTIONS -----------------
void playTone(int freq, int durationMs, bool specialEffect) {
  currentBaseFreq = freq;
  applyVibrato = specialEffect;
  
  if (freq == 0) {
    ledcWrite(0, 0); // Matikan suara di Channel 0 jika freq = 0
  } else {
    ledcWriteTone(0, freq);
    ledcWrite(0, 80); // Volume lembut (duty cycle 80 dari 255)
  }
  
  buzzerEndTime = millis() + durationMs;
  buzzerActive = true;
}

void updateBuzzer() {
  if (buzzerActive) {
    if (millis() >= buzzerEndTime) {
      ledcWrite(0, 0); // Matikan suara di Channel 0
      buzzerActive = false;
    } else if (applyVibrato && currentBaseFreq > 0) {
      float angle = (millis() % 150) * (2.0 * 3.14159 / 150.0);
      int smoothPitchShift = sin(angle) * 3; 
      
      ledcWriteTone(0, currentBaseFreq + smoothPitchShift);
      ledcWrite(0, 80); // Pastikan volume tetap terjaga
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

    if (tuneIndex == SONG_LENGTH - 1) {
      noteDuration *= 1;
    }

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

void armToNextRound(unsigned long currentTime) {
  gameMode = MODE_ARMED;
  armedStartTime = currentTime;
  armedTargetWait = random(currentWaitTime * 0.6, currentWaitTime + 1);
  offsetX = 0;
  offsetY = 0;
  targetOffsetX = 0;
  targetOffsetY = 0;
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
    } else if (idleDisplayMode == 1) {
      drawMatrixContent(currentTime);
    } else if (idleDisplayMode == 2) {
      drawAudioVisualizerContent(currentTime);
    } else if (idleDisplayMode == 3) {
      drawClockContent(currentTime);
    }
  } else {
    if (currentSelectedMenu == 0) {
      drawEyesNormal(-10, 0, blinkState == 1);
      drawHint("[ REACTION GAME ]");
    } else if (currentSelectedMenu == 1) { 
      int danceY = (millis() % 400 < 200) ? 2 : -2;
      drawEyesHappyAnimation(danceY);
      drawHint("[ JUKEBOX MUSIC ]");
    } else if (currentSelectedMenu == 2) { 
      drawEyesSurprised();
      drawHint("[ WI-FI GRAPH ]");
    } else if (currentSelectedMenu == 3) { 
      display.drawRect(leftEyeX, eyeY + 5, eyeWidth, 12, WHITE);
      display.drawRect(rightEyeX, eyeY + 5, eyeWidth, 12, WHITE);
      display.fillRect(leftEyeX + 4, eyeY + 10, 4, 2, WHITE);
      display.fillRect(rightEyeX + 4, eyeY + 10, 4, 2, WHITE);
      drawHint("[ SYS MONITOR ]");
    } else if (currentSelectedMenu == 4) { 
      display.fillTriangle(leftEyeX + 5, eyeY + 15, leftEyeX + 20, eyeY + 5, leftEyeX + 20, eyeY + 25, WHITE);
      display.fillTriangle(rightEyeX + 5, eyeY + 15, rightEyeX + 20, eyeY + 5, rightEyeX + 20, eyeY + 25, WHITE);
      drawHint("[ FLASHLIGHT ]");
    }

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

void runStealthLightMode(unsigned long currentTime) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(22, 2);
  display.print("= FLASHLIGHT =");

  display.setCursor(16, 22);
  display.print("LED Power : ");
  display.print(blueLedState ? "ON " : "OFF");

  display.setCursor(16, 34);
  display.print("LED Mode  : ");
  display.print(ledStrobeMode ? "STROBE (BLINK)" : "STEADY");

  display.setCursor(16, 56);
  display.print("[tap once to exit]");
  display.display();

  if (touchTapEvent) {
    touchTapEvent = false;
    blueLedState = false;
    ledStrobeMode = false;
    digitalWrite(BLUE_LED_PIN, LOW);
    playTone(1200, 60);
    gameMode = MODE_IDLE;
  }

  if (touchConfirmEvent) {
    touchConfirmEvent = false;
    ledStrobeMode = !ledStrobeMode;
    if (!ledStrobeMode) {
      digitalWrite(BLUE_LED_PIN, HIGH);
    }
    playTone(ledStrobeMode ? 2200 : 1500, 80);
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
  display.print("= SOVIET ANTHEM =");

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

void runArmed(unsigned long currentTime) {
  if (touchTapEvent) {
    touchTapEvent = false;
    triggerMiss(currentTime);
    return;
  }

  if (currentTime - armedStartTime >= armedTargetWait) {
    gameMode = MODE_ALERT;
    alertStartTime = currentTime;
    playTone(950, 60);
  }

  display.clearDisplay();
  drawEyesNormal(0, 0, false);
  drawHud();
  display.display();
}

void runAlert(unsigned long currentTime) {
  if (touchTapEvent) {
    touchTapEvent = false;
    triggerHit(currentTime);
    return;
  }
  if (currentTime - alertStartTime >= REACTION_WINDOW) {
    triggerMiss(currentTime);
    return;
  }

  display.clearDisplay();
  drawEyesSurprised();
  drawHud();
  display.display();
}

void triggerHit(unsigned long currentTime) {
  score++;
  if (currentWaitTime > MIN_WAIT_TIME) {
    currentWaitTime -= 120;
    if (currentWaitTime < MIN_WAIT_TIME) currentWaitTime = MIN_WAIT_TIME;
  }
  playTone(1050, 80);
  gameMode = MODE_HIT;
  feedbackShownAt = currentTime;
}

void triggerMiss(unsigned long currentTime) {
  lives--;
  playTone(180, 200);
  gameMode = MODE_MISS;
  feedbackShownAt = currentTime;
}

void runFeedback(unsigned long currentTime, bool isHit) {
  display.clearDisplay();
  if (isHit) {
    drawEyesHappy();
  } else {
    drawEyesSad();
  }
  drawHud();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(40, 2);
  display.print(isHit ? "+1!" : "MISS");
  display.display();

  if (currentTime - feedbackShownAt >= FEEDBACK_DURATION) {
    if (lives <= 0) {
      startGameOver(currentTime);
    } else {
      armToNextRound(currentTime);
    }
  }
}

void startGameOver(unsigned long currentTime) {
  gameMode = MODE_GAMEOVER;
  ledcWrite(0, 0);
  playTone(440, 150);
  delay(160);
  playTone(290, 300);
}

void runGameOver(unsigned long currentTime) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(15, 15);
  display.print("GAME OVER");
  display.setTextSize(1);
  display.setCursor(30, 40);
  display.print("Score: ");
  display.print(score);
  display.setCursor(10, 54);
  display.print("[tap to return]");
  display.display();

  if (touchTapEvent) {
    touchTapEvent = false;
    gameMode = MODE_IDLE;
  }
}

// ----------------- PURE GRAPHIC RENDERERS -----------------
void drawHud() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("S:");
  display.print(score);
  display.setCursor(90, 0);
  display.print("L:");
  display.print(lives);
}

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