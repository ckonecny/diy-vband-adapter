// DIY VBAND Adapter - Waveshare RP2040-Zero version
//
// Turns a Waveshare RP2040-Zero (or a compatible clone) into a USB keyboard
// that reports the two contacts of a Morse paddle (or a straight key) as key
// presses, the same way the commercial vband USB adapter does:
//
//   dit (jack tip)  -> Left Ctrl
//   dah (jack ring) -> Right Ctrl
//
// The keyer logic (iambic A/B, ultimatic, bug, ...) lives in the application
// (https://hamradio.solutions/vband/, APEX CW, ...). This firmware only
// mirrors the contact state, with a short debounce and no added delay.
//
// This is the sibling of firmware/xiao_samd21/. The logic is identical; only
// the pins, the board core and the LED handling differ.
//
// Build with the Arduino-Pico core by Earle Philhower. Its built-in Keyboard
// library is used (do NOT install the "Keyboard" library by Arduino for this
// board). The only extra library is Adafruit NeoPixel for the on-board RGB LED.
//
// See README.md for wiring and configuration.

#include <Keyboard.h>

// ---------------------------------------------------------------------------
// Configuration (edit here or override with -D build flags)
// ---------------------------------------------------------------------------

// Input pins (GPIO numbers, as printed on the board). The jack sleeve goes to
// GND, a closed contact pulls the pin LOW.
#ifndef PIN_DIT
#define PIN_DIT 1   // GP1 <- jack tip (same number as XIAO D1)
#endif
#ifndef PIN_DAH
#define PIN_DAH 2   // GP2 <- jack ring (same number as XIAO D2)
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

// On-board LED. The RP2040-Zero has a WS2812 RGB LED on GP16 instead of a
// plain LED. Set USE_RGB_LED to 0 to leave it dark (and drop the NeoPixel
// library). LED_BRIGHTNESS is 0..255; the LED is very bright at full power.
#ifndef USE_RGB_LED
#define USE_RGB_LED 1
#endif
#ifndef PIN_RGB_LED
#define PIN_RGB_LED 16
#endif
#ifndef LED_BRIGHTNESS
#define LED_BRIGHTNESS 20
#endif

// ---------------------------------------------------------------------------

#if USE_RGB_LED
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixel(1, PIN_RGB_LED, NEO_GRB + NEO_KHZ800);
const uint32_t LED_KEYING = 0x00FF00;  // green while a paddle is closed
const uint32_t LED_READY = 0x0000FF;   // blue blink: ready / swapped
const uint32_t LED_OFF = 0x000000;

void setLed(uint32_t color) {
  pixel.setPixelColor(0, color);
  pixel.show();
}
#else
const uint32_t LED_KEYING = 0;
const uint32_t LED_READY = 0;
const uint32_t LED_OFF = 0;

void setLed(uint32_t) {}
#endif

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

// Explicit prototypes: PlatformIO's .ino conversion would otherwise put the
// generated ones above struct Contact.
bool readRaw(const Contact &c);
void report(Contact &c, bool down);
void update(Contact &c, uint32_t now);

bool ledOn = false;

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
    setLed(LED_READY);
    delay(120);
    setLed(LED_OFF);
    delay(120);
  }
}

void setup() {
  pinMode(PIN_DIT, INPUT_PULLUP);
  pinMode(PIN_DAH, INPUT_PULLUP);
#if USE_RGB_LED
  pixel.begin();
  pixel.setBrightness(LED_BRIGHTNESS);
#endif
  setLed(LED_OFF);
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

  // Only touch the LED when the state changes: updating a WS2812 briefly
  // blocks interrupts and there is no reason to do it on every loop pass.
  bool keying = dit.reported || dah.reported;
  if (keying != ledOn) {
    ledOn = keying;
    setLed(keying ? LED_KEYING : LED_OFF);
  }
}
