# PPMReaderRP2040

A lightweight **PPM (Pulse Position Modulation)** signal decoder for **RP2040** microcontrollers (Raspberry Pi Pico, RP2040 Zero, etc.).

This is a personal rewrite of the classic Arduino PPM reader libraries — adapted specifically for the RP2040's interrupt handling, with modern C++ features and a clean callback-based API.

## Why this library?

Existing PPM libraries were written for AVR and often rely on pin-change interrupts or platform-specific assumptions. The RP2040 has a different interrupt architecture. This library:

- Uses standard `attachInterrupt()` (RISING edge) — compatible with RP2040
- Provides **connection/disconnection monitoring** (500ms timeout)
- Provides `onNewData()` callbacks with consistent snapshots of all channel values
- Supports `latestValidChannelValue()` with fallback defaults — ideal for failsafe
- Non-blocking: ISR is minimal, all callback execution happens in `loop()`

## Pin Connections

| Signal  | RP2040 Pin |
|---------|------------|
| PPM IN  | GPIO2      |
| GND     | GND        |
| 5V (or 3.3V) | 3.3V or 5V (check your receiver) |

> **Note:** The PPM signal from most RC receivers is 5V logic. The RP2040 is 3.3V tolerant on GPIO pins, but **5V input may damage the pin**. Use a voltage divider (e.g., 1kΩ + 2.2kΩ) or a logic level converter if your receiver outputs 5V.

## Usage

```cpp
#include <PPMReaderRP2040.h>

// Pin 2, 10 channels
PPMReader ppm(2, 10);

void setup() {
    Serial.begin(115200);
    
    // Optional: customize bounds
    ppm.minChannelValue = 990;
    ppm.maxChannelValue = 2010;
    ppm.channelValueMaxError = 15;
    
    // Optional: callbacks
    ppm.onConnect([]() {
        Serial.println("PPM connected!");
    });
    ppm.onDisconnect([]() {
        Serial.println("PPM signal lost — failsafe!");
    });
    ppm.onNewData([](const unsigned long* values, byte numChannels) {
        // Called when a full valid frame is received
        // values[0] = channel 1, etc.
    });
}

void loop() {
    ppm.loop();  // must be called regularly
    
    // Non-blocking read with failsafe defaults
    unsigned long ch1 = ppm.latestValidChannelValue(1, 1500);
    unsigned long ch3 = ppm.latestValidChannelValue(3, 1000);  // throttle
    
    // ... do something with values
}
```

## API

### Constructor
- `PPMReader(byte interruptPin, byte channelAmount)` — attach to pin, expect N channels

### Configuration (public fields)
- `minChannelValue` / `maxChannelValue` — valid range (default: 1000–2000)
- `channelValueMaxError` — tolerance (default: 10)
- `blankTime` — frame sync threshold (default: 2100µs)
- `microsAtLastPulse` — timestamp of last interrupt (volatile)

### Channel Reading
- `unsigned long rawChannelValue(byte channel)` — latest raw value (1-indexed)
- `unsigned long latestValidChannelValue(byte channel, unsigned long defaultValue)` — last validated value, or fallback

### Callbacks
- `onConnect(PPMConnectCallback)` — called when signal first appears
- `onDisconnect(PPMDisconnectCallback)` — called after 500ms no signal
- `onNewData(PPMNewDataCallback)` — called with snapshot of all valid values each frame

### Housekeeping
- `void loop()` — call each iteration to check connection status

## License

MIT — do what you want.
