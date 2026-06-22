#include <Adafruit_NeoPixel.h>
#include "LEDMatrix.h"
#include "GameManager.h"

#define LED_PIN         6
#define LED_BRIGHTNESS 20
#define JOY_X          A0
#define JOY_Y          A1
#define JOY_BTN         7
#define RESET_BTN       2      // D2, Taster gegen GND (INPUT_PULLUP, LOW = gedrückt)
#define JOY_DEADZONE  200
#define JOY_DELAY_MS  400
#define JOY_REPEAT_MS 180

// Doppelklick: 2 Drücke innerhalb 3 Sekunden = Reset
#define RESET_WINDOW_MS 3000
// Entprell-Zeit: Signal muss 20ms stabil sein
#define DEBOUNCE_MS      20

LEDMatrix   matrix(LED_PIN, LED_BRIGHTNESS);
GameManager gm(matrix);

// Joystick
bool     btnLast      = false;
uint32_t lastMoveTime = 0;
bool     firstMove    = true;

// Reset-Taster D2
uint8_t  resetState      = HIGH;  // letzter entprellter Zustand (HIGH = offen)
uint8_t  resetRawLast    = HIGH;  // rohes Signal vom letzten Frame
uint32_t resetDebounceT  = 0;     // wann hat sich das Roh-Signal zuletzt geändert
uint8_t  resetClickCount = 0;     // 0 oder 1 (erster Klick gezählt)
uint32_t resetFirstClick = 0;     // Zeitstempel des ersten Klicks

void setup() {
  pinMode(JOY_BTN,   INPUT_PULLUP);
  pinMode(RESET_BTN, INPUT_PULLUP);  // interner Pullup, Taster gegen GND
  matrix.begin();
  gm.begin();
}

void loop() {
  uint32_t now = millis();

  // ── Reset D2: Entprellung + Doppelklick ──────────────────
  uint8_t resetRaw = digitalRead(RESET_BTN);

  // Entprellung: nur übernehmen wenn Signal DEBOUNCE_MS stabil
  if (resetRaw != resetRawLast) {
    resetDebounceT = now;        // neue Flanke → Timer neu starten
    resetRawLast   = resetRaw;
  }
  bool resetPressed = false;
  if ((now - resetDebounceT) >= DEBOUNCE_MS && resetRaw != resetState) {
    resetState = resetRaw;
    if (resetState == LOW) {     // stabile fallende Flanke = Tastendruck
      resetPressed = true;
    }
  }

  if (resetPressed) {
    if (resetClickCount == 0) {
      // Erster Klick: merken und warten
      resetClickCount = 1;
      resetFirstClick = now;
      // Kurzer Blink als visuelles Feedback: eine LED leuchtet kurz auf
      matrix.setPixel(0, 0, 0xFFFF00);
      matrix.show();
      delay(80);
    } else {
      // Zweiter Klick
      if ((now - resetFirstClick) <= RESET_WINDOW_MS) {
        // Doppelklick erkannt → Reset
        gm.resetCurrentGame();
      }
      // In jedem Fall zurücksetzen (egal ob Fenster abgelaufen oder nicht)
      resetClickCount = 0;
    }
  }

  // Timeout: erster Klick verfällt nach RESET_WINDOW_MS
  if (resetClickCount > 0 && (now - resetFirstClick) > RESET_WINDOW_MS) {
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
