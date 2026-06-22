#include <Adafruit_NeoPixel.h>
#include "LEDMatrix.h"
#include "GameManager.h"

#define LED_PIN         6
#define LED_BRIGHTNESS 20
#define JOY_X          A0
#define JOY_Y          A1
#define JOY_BTN         7
#define RESET_BTN       2
#define JOY_DEADZONE  200
#define JOY_DELAY_MS  400
#define JOY_REPEAT_MS 180
#define RESET_WINDOW_MS 5000

LEDMatrix   matrix(LED_PIN, LED_BRIGHTNESS);
GameManager gm(matrix);

bool     btnLast      = false;
uint32_t lastMoveTime = 0;
bool     firstMove    = true;

// ── Reset-Taster D2 ──────────────────────────────────────────
// Einfache Flanken-Erkennung ohne delay() — funktioniert zuverlässig
// Taster: D2 gegen GND, INPUT_PULLUP → LOW wenn gedrückt
bool     resetPrev       = HIGH;   // letzter stabiler Zustand (HIGH = offen)
uint8_t  resetClickCount = 0;
uint32_t resetFirstClick = 0;

void setup() {
  pinMode(JOY_BTN,   INPUT_PULLUP);
  pinMode(RESET_BTN, INPUT_PULLUP);
  matrix.begin();
  gm.begin();
}

void loop() {
  uint32_t now = millis();

  // ── D2: einfache Flanken-Erkennung (LOW = gedrückt) ──────
  bool resetNow = digitalRead(RESET_BTN);  // HIGH oder LOW
  if (resetPrev == HIGH && resetNow == LOW) {
    // Fallende Flanke = Knopf wurde gedrückt
    if (resetClickCount == 0) {
      resetClickCount = 1;
      resetFirstClick = now;
    } else if (now - resetFirstClick <= RESET_WINDOW_MS) {
      gm.resetCurrentGame();
      resetClickCount = 0;
    } else {
      resetClickCount = 1;
      resetFirstClick = now;
    }
  }
  resetPrev = resetNow;

  if (resetClickCount > 0 && (now - resetFirstClick > RESET_WINDOW_MS))
    resetClickCount = 0;

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
