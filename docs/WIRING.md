# Wiring

Only three wires are needed: the two paddle contacts and their common ground.
The board's internal pull-up resistors keep the inputs HIGH; closing a paddle
contact pulls the input to GND. This applies to both supported boards, the
Seeed Studio XIAO SAMD21 and the Waveshare RP2040-Zero.

## Connections

| 3.5 mm jack   | Paddle      | XIAO pin | RP2040-Zero pin |
|---------------|-------------|----------|-----------------|
| **Tip**       | Dit         | **D1**   | **GP1**         |
| **Ring**      | Dah         | **D2**   | **GP2**         |
| **Sleeve**    | Common      | **GND**  | **GND**         |

This follows the usual convention for paddles (and the vband adapter):
tip = dit, ring = dah, sleeve = common. If your paddle is wired the other way
round, swap in software (see the README) instead of rewiring.

## XIAO SAMD21 pinout

Top view, USB‑C connector at the top:

```
              ┌──[USB-C]──┐
   A0 / D0  ──┤ 1      14 ├──  5V      (do not use)
   A1 / D1  ──┤ 2      13 ├──  GND     ◄── jack SLEEVE
   A2 / D2  ──┤ 3      12 ├──  3V3     (do not use)
   A3 / D3  ──┤ 4      11 ├──  D10
   A4 / D4  ──┤ 5      10 ├──  D9
   A5 / D5  ──┤ 6       9 ├──  D8
   A6 / D6  ──┤ 7       8 ├──  D7
              └───────────┘
      D1 ◄── jack TIP  (dit)
      D2 ◄── jack RING (dah)
```

## Waveshare RP2040-Zero pinout

The RP2040-Zero has several GND pads and its GPIO pads are labelled on the
silkscreen (`GP0`, `GP1`, `GP2`, …). Use the labels printed on your board, and
check them against the
[Waveshare pinout](https://www.waveshare.com/wiki/RP2040-Zero), because clones
differ in layout details.

| Function        | Pad        | Notes                                    |
|-----------------|------------|------------------------------------------|
| Dit (jack tip)  | `GP1`      | Change with `PIN_DIT` in the sketch      |
| Dah (jack ring) | `GP2`      | Change with `PIN_DAH` in the sketch      |
| Common (sleeve) | any `GND`  | Use the GND pad closest to GP1/GP2       |

GP1 and GP2 sit next to each other on the edge of the board, so the jack
wires stay short. Any other free GPIO works too (GP0–GP15, GP26–GP29): change
`PIN_DIT` / `PIN_DAH` accordingly.

The board has two small buttons: **BOOT** (hold while connecting USB to enter
the bootloader) and **RESET**. There is no RST pad pair to short as on the
XIAO. Leave `5V` and `3V3` unconnected.

The on-board LED is a WS2812 RGB LED on `GP16`. It is not a plain LED, so do
not use that pad for anything else.

## 3.5 mm plug and jack

```
   Plug:   ┌──────────┬──┬──────┬──┬────┐
           │  Sleeve  │▓▓│ Ring │▓▓│Tip >
           └──────────┴──┴──────┴──┴────┘
             common       dah       dit
```

### Jack breakout boards

Small TRS breakout boards are usually labelled for audio:

| Board label           | Contact | Connect to (XIAO / RP2040-Zero) |
|-----------------------|---------|---------------------------------|
| `L` / `LEFT` / `T`    | Tip     | D1 / GP1                        |
| `R` / `RIGHT` / `R1`  | Ring    | D2 / GP2                        |
| `G` / `GND` / `S`     | Sleeve  | GND / GND                       |

Some boards and bare jacks have extra pins (switch contacts that open when a
plug is inserted, or a second ring pin on TRRS jacks). Leave those
unconnected.

**Always verify with a multimeter:** insert the paddle plug, set the meter to
continuity, and check which board pad beeps when you press dit and which when
you press dah (the other probe on the sleeve pad). Labels on cheap breakout
boards are not always right.

### Bare panel jack

Look at the jack from the solder side and use continuity again: plug in a
cable, then find the lug connected to the plug's tip, ring and sleeve. The
sleeve lug is usually the one connected to the metal bushing / front nut.

## Optional protection

The direct connection works and is what most DIY adapters use. If the paddle
cable is long or you want some protection against static discharge, add a
**220 Ω … 1 kΩ** resistor in series with each input:

```
   jack TIP  ──[ 220 Ω ]── D1  (RP2040-Zero: GP1)
   jack RING ──[ 220 Ω ]── D2  (RP2040-Zero: GP2)
   jack SLEEVE ──────────── GND
```

The internal pull-ups (≈ 40 kΩ on the XIAO, ≈ 50–80 kΩ on the RP2040) are
much larger, so the logic levels are not affected.

## Do not

- **Do not** connect 5V or 3V3 to the jack. The inputs only need to be pulled
  to GND by the paddle contacts.
- **Do not** connect the output of an external keyer, transceiver or any
  circuit that drives a voltage. The SAMD21 and the RP2040 are 3.3 V parts and
  their pins are **not 5 V tolerant**.
- Avoid the XIAO's back-side pads (battery, SWD) unless you know what you are
  doing.

## Example build

A compact build without an enclosure (shown with the XIAO): a small 3.5 mm
jack mounted on the back of the board, in line with the USB‑C connector,
three short wires to D1, D2 and GND, and the whole thing wrapped in clear
heat-shrink tubing. The tubing keeps the LEDs visible. The RP2040-Zero is
about the same size and can be built the same way.

![Finished adapter, top side](images/adapter-top.jpg)

![Finished adapter, jack side](images/adapter-jack-side.jpg)

## Enclosure tips

- Glue or screw the jack to the enclosure so that plugging in does not stress
  the solder joints.
- Leave the USB‑C connector reachable, plus the RST pads (XIAO) or the BOOT
  button (RP2040-Zero), or at least keep the lid removable for firmware
  updates.
- A small window or light pipe over the LED is handy to see the keying state.

## Quick test after wiring

1. Plug in the paddle, then the USB cable. The LED blinks once (XIAO: yellow,
   RP2040-Zero: blue).
2. Press dit: the LED lights up (RP2040-Zero: green). Same for dah.
3. Open <https://w3c.github.io/uievents/tools/key-event-viewer.html> and
   check: dit → `ControlLeft`, dah → `ControlRight`.
