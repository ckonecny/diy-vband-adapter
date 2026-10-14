// DIY VBAND Adapter - Seeed Studio XIAO SAMD21 version
//
// Turns a Seeed Studio XIAO SAMD21 into a USB keyboard that reports the two
// contacts of a Morse paddle (or a straight key) as key presses, the same way
// the commercial vband USB adapter does:
//
//   dit (jack tip)  -> Left Ctrl
//   dah (jack ring) -> Right Ctrl
//
// The keyer logic (iambic A/B, ultimatic, bug, ...) lives in the application
// (https://hamradio.solutions/vband/, APEX CW, ...). This firmware
// only mirrors the contact state, with a short debounce and no added delay.
//
// A sibling sketch for the Waveshare RP2040-Zero lives in
// firmware/rp2040_zero/. See README.md for wiring and configuration.

#include <Keyboard.h>

// ---------------------------------------------------------------------------
// Configuration (edit here or override with -D build flags)
// ---------------------------------------------------------------------------

// Input pins. The jack sleeve goes to GND, a closed contact pulls the pin LOW.
#ifndef PIN_DIT
#define PIN_DIT 1   // XIAO D1 <- jack tip
#endif
#ifndef PIN_DAH
#define PIN_DAH 2   // XIAO D2 <- jack ring
#endif

// Key mapping. Ctrl keys are independent of the host keyboard layout, which is
// why they are the default (the original vband adapter uses them as well).
// vband also accepts '[' and ']', but those only arrive correctly on hosts set
// to a US layout, because the Keyboard library sends US key positions.
#ifndef KEY_DIT
#define KEY_DIT KEY_LEFT_CTRL
#endif
#ifndef KEY_DAH
#define KEY_DAH KEY_RIGHT_CTRL
#endif

// Swap dit and dah (left-handed use or reversed wiring). Can also be toggled
// at power-up by holding the dit paddle, see README.md.
#ifndef SWAP_PADDLES
#define SWAP_PADDLES 0
#endif

// A contact must be stable this long before a change is reported.
// 5 ms is well below a dit at 50 WPM (24 ms) but filters contact bounce.
#ifndef DEBOUNCE_MS
#define DEBOUNCE_MS 5
#endif

// A contact closed longer than this is treated as stuck (e.g. a mono straight
// key plug shorting the ring to the sleeve): its key is released and the
// contact is ignored until it opens again. Keeps Ctrl from being held forever.
#ifndef STUCK_MS
#define STUCK_MS 10000
#endif

// ---------------------------------------------------------------------------

// The XIAO SAMD21 user LED (yellow, "L") is active LOW.
const uint8_t LED_ON = LOW;
const uint8_t LED_OFF = HIGH;

struct Contact {
  uint8_t pin;
  uint8_t key;
  bool pressed;       // debounced contact state
  bool reported;      // key is currently held on the host
  bool ignored;       // stuck, wait until the contact opens
  bool lastRaw;       // last raw reading
  uint32_t changedAt; // millis() of the last raw change
};

Contact dit = {PIN_DIT, KEY_DIT, false, false, false, false, 0};
Contact dah = {PIN_DAH, KEY_DAH, false, false, false, false, 0};

bool readRaw(const Contact &c) {
  return digitalRead(c.pin) == LOW;
}

void report(Contact &c, bool down) {
  if (down == c.reported) {
    return;
  }
  c.reported = down;
  if (down) {
    Keyboard.press(c.key);
  } else {
    Keyboard.release(c.key);
  }
}

void update(Contact &c, uint32_t now) {
  bool raw = readRaw(c);
  if (raw != c.lastRaw) {
    c.lastRaw = raw;
    c.changedAt = now;
  }
  uint32_t stableFor = now - c.changedAt;

  if (raw != c.pressed && stableFor >= DEBOUNCE_MS) {
    c.pressed = raw;
    if (!raw) {
      c.ignored = false;
    }
  }
  if (c.pressed && stableFor >= STUCK_MS) {
    c.ignored = true;
  }
  report(c, c.pressed && !c.ignored);
}

void blink(uint8_t times) {
  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, LED_ON);
    delay(120);
    digitalWrite(LED_BUILTIN, LED_OFF);
    delay(120);
  }
}

void setup() {
  pinMode(PIN_DIT, INPUT_PULLUP);
  pinMode(PIN_DAH, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);
  delay(10); // let the pull-ups settle

  // Holding the dit paddle while plugging in USB swaps dit and dah until the
  // next power cycle (two blinks). Otherwise one blink signals "ready".
  bool swap = SWAP_PADDLES;
  if (readRaw(dit)) {
    swap = !swap;
  }
  if (swap) {
    uint8_t k = dit.key;
    dit.key = dah.key;
    dah.key = k;
  }

  // A contact already closed at power-up (the held dit paddle, or the ring of
  // a mono plug) starts out ignored until it opens.
  dit.ignored = dit.pressed = dit.lastRaw = readRaw(dit);
  dah.ignored = dah.pressed = dah.lastRaw = readRaw(dah);

  Keyboard.begin();
  Keyboard.releaseAll();

  blink(swap ? 2 : 1);
}

void loop() {
  uint32_t now = millis();
  update(dit, now);
  update(dah, now);
  digitalWrite(LED_BUILTIN, (dit.reported || dah.reported) ? LED_ON : LED_OFF);
}
