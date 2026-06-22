#include <Adafruit_NeoPixel.h>
#include "LEDMatrix.h"
#include "GameManager.h"

#define LED_PIN        6
#define LED_BRIGHTNESS 20
#define JOY_X          A0
#define JOY_Y          A1
#define JOY_BTN        7
#define RESET_BTN      2    // D2: Reset-Taster (doppelt drücken in 5s)
#define JOY_DEADZONE   200
#define JOY_DELAY_MS   400
#define JOY_REPEAT_MS  180

// Doppelklick-Reset: zwei Drücke innerhalb RESET_WINDOW_MS = Neustart
#define RESET_WINDOW_MS 5000

LEDMatrix   matrix(LED_PIN, LED_BRIGHTNESS);
GameManager gm(matrix);

bool     btnLast      = false;
uint32_t lastMoveTime = 0;
bool     firstMove    = true;

// Reset-Taster Zustand
bool     resetBtnLast    = false;
uint8_t  resetClickCount = 0;
uint32_t resetFirstClick = 0;

void setup() {
  pinMode(JOY_BTN,   INPUT_PULLUP);
  pinMode(RESET_BTN, INPUT_PULLUP);
  matrix.begin();
  gm.begin();
}

void loop() {
  // ── Reset-Taster (D2) ────────────────────────────────────
  bool resetNow     = (digitalRead(RESET_BTN) == LOW);
  bool resetPressed = (resetNow && !resetBtnLast);
  resetBtnLast = resetNow;

  if (resetPressed) {
    uint32_t now = millis();
    if (resetClickCount == 0) {
      // Erster Klick: Zeitfenster starten
      resetClickCount = 1;
      resetFirstClick = now;
    } else {
      // Zweiter Klick: prüfen ob innerhalb 5s
      if (now - resetFirstClick <= RESET_WINDOW_MS) {
        // Doppelklick erkannt → Spiel neu starten
        gm.resetCurrentGame();
        resetClickCount = 0;
      } else {
        // Fenster abgelaufen: neu beginnen
        resetClickCount = 1;
        resetFirstClick = now;
      }
    }
  }

  // Zeitfenster automatisch zurücksetzen wenn abgelaufen
  if (resetClickCount > 0 && (millis() - resetFirstClick > RESET_WINDOW_MS)) {
    resetClickCount = 0;
  }

  // ── Joystick ─────────────────────────────────────────────
  int vx = analogRead(JOY_X);
  int vy = analogRead(JOY_Y);

  int8_t dx = 0, dy = 0;
  if (vx < 512 - JOY_DEADZONE) dx = -1;
  if (vx > 512 + JOY_DEADZONE) dx =  1;
  if (vy < 512 - JOY_DEADZONE) dy = -1;
  if (vy > 512 + JOY_DEADZONE) dy =  1;

  bool btnNow     = (digitalRead(JOY_BTN) == LOW);
  bool btnPressed = (btnNow && !btnLast);
  btnLast = btnNow;

  bool moved = false;
  if (dx != 0 || dy != 0) {
    uint32_t now     = millis();
    uint32_t waitFor = firstMove ? JOY_DELAY_MS : JOY_REPEAT_MS;
    if (lastMoveTime == 0 || now - lastMoveTime >= waitFor) {
      moved        = true;
      lastMoveTime = now;
      firstMove    = false;
    }
  } else {
    lastMoveTime = 0;
    firstMove    = true;
  }

  gm.update(moved ? dx : 0, moved ? dy : 0, btnPressed);
  delay(16);
}
