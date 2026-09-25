# 433 MHz RF Relay Receiver (4 Channels)

Arduino firmware that receives commands from a 433 MHz ASK remote and drives four relays. Each remote button can be mapped to any combination of relays and assigned its own switching behaviour: toggle, momentary, or timed.

This is the receiver half of a pair. The matching remote firmware is in [emeteur_433mhz_rf-interrupteur_4_pos_arduino_code](https://github.com/GUELORD-MWENDERWA/emeteur_433mhz_rf-interrupteur_4_pos_arduino_code).

## Features

- Frame validation: transmitter ID whitelist, button index bounds check, and duplicate suppression by sequence number
- Configurable button-to-relay matrix (one button can drive several relays)
- Four switching modes per button:
  - `TOGGLE` flips the relay state on each press
  - `MOMENTANE` (momentary) follows the button: on while pressed, off when released
  - `TEMPORISE_EXCITATION` (on-delay pulse) switches the relay on for a set duration, then restores its default state
  - `TEMPORISE_DESEXCITATION` (off-delay pulse) switches the relay off for a set duration, then restores its default state
- Per-relay power-on default state
- Support for active-high or active-low relay boards
- Non-blocking timers based on `millis()`

## Hardware

| Component | Notes |
| --- | --- |
| Arduino Uno / Nano (ATmega328P) | |
| 433 MHz ASK receiver module (XY-MK-5V or equivalent) | Data pin on D11 (RadioHead `RH_ASK` default) |
| 4-channel relay board | D6, D7, D8, D9 |

## Protocol

The receiver expects frames in the format produced by the transmitter:

```
<TX_ID>|B<n>|<ON|OFF>|<seq>
```

Frames with an unknown ID (`ID_AUTORISE`, default `TX1`), a malformed button field, or a sequence number equal to the last one processed are ignored.

## Configuration

All behaviour is defined by constants at the top of `RECEPTEUR_cmd_relais.ino`:

| Setting | Purpose |
| --- | --- |
| `relaisPins` | Relay output pins |
| `RELAY_ACTIVE_HIGH` | Set to `false` for active-low relay boards |
| `mapping[4][4]` | Button-to-relay matrix (`1` = controlled) |
| `defaultRelayState` | Relay state at power-on and after a timed pulse |
| `modeBouton` | Switching mode per button |
| `tempoBouton` | Pulse duration in milliseconds for timed modes |
| `ID_AUTORISE` | Accepted transmitter ID |

## Build and upload

1. Install the [RadioHead](https://www.airspayce.com/mikem/arduino/RadioHead/) library.
2. Open `RECEPTEUR_cmd_relais.ino` in the Arduino IDE.
3. Adjust the configuration block for your wiring, then upload.
4. Open the serial monitor at 9600 baud to see received frames.

## Safety

Relay boards switching mains voltage must be installed in an enclosure by a qualified person. Test the logic with low-voltage loads first.

## License

No license has been specified yet. Contact the author before reusing this code.
