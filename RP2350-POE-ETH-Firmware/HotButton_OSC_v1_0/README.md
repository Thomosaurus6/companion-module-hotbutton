# HotButton Firmware

Firmware for the **HotButton PoE network button**, based on the **Waveshare RP2350-POE-ETH**.

This firmware is designed to work together with the HotButton module for **Bitfocus Companion** contained in this repository.

---

## Version

**HotButton Firmware v1.0**

Arduino sketch: `HotButton_OSC_v1_0.ino`

## Hardware

| Component | Configuration |
| --- | --- |
| Controller | Waveshare RP2350-POE-ETH |
| MCU | RP2350A |
| Ethernet | WIZnet W6300 |
| Power | PoE |
| Button | GPIO0 → GND |
| External WS2812 | GPIO1 |
| Onboard WS2812 | GPIO25 |

The external WS2812 LED is optional. The HotButton operates normally without an LED connected to GPIO1.

## Features

- DHCP and static IPv4 operation
- Persistent Companion pairing
- Broadcast discovery while unpaired
- Unicast OSC communication after pairing
- Physical Press and Release events
- Heartbeat / online monitoring
- Companion-controlled RGB LED
- LED brightness and flash programs
- LED state synchronization
- Physical network and pairing recovery
- Optional serial debugging

Long-press detection is intentionally handled by the **Companion module**, not by the firmware.

## Ethernet / W6300 Driver

The firmware uses the Waveshare/WIZnet W6300 driver sources included in the local `src` directory.

> **Important:** Do not replace the included W6300 driver files with the unmodified Waveshare/WIZnet versions without reviewing the changes.
>
> The included `socket.c` contains a required fix for **non-blocking UDP socket operation**. Without this fix, the OSC receive loop can block after startup and prevent further button, heartbeat and OSC processing.

The required driver files are included with the firmware so that the tested v1.0 implementation remains self-contained.

## Network Behaviour

### First Start

The HotButton initially starts using **DHCP**.

While the device is unpaired, OSC discovery traffic is sent using directed broadcast. After successful pairing with Companion, the Companion IP address is stored persistently and normal communication switches to **unicast UDP**.

Pairing therefore survives a normal HotButton reboot or power cycle.

### Static IP

A static IPv4 configuration can be sent from the Companion module. Supported parameters are:

- IP address
- Subnet mask
- Gateway

A gateway of `0.0.0.0` disables gateway monitoring.

Changing the HotButton's own IP configuration does **not** clear its stored Companion pairing.

## Physical Recovery

A network and pairing recovery is available during the **first 30 seconds after boot**.

Hold the physical button continuously for **5 seconds** during this window.

This clears:

- Stored static network configuration
- Stored Companion pairing

The HotButton then returns to **DHCP**, an **unpaired state**, and **broadcast discovery**.

After the initial 30-second boot window, the recovery function is disabled until the next reboot. Normal button operation continues unaffected.

This is intentionally **not a full factory reset** of unrelated settings.

## OSC Communication

Default UDP port: **13122**

### Button Events

- `/<device-id>_press`
- `/<device-id>_release`

The firmware additionally implements OSC messages for:

- Heartbeat
- Pairing and pairing acknowledgement
- LED control
- LED state synchronization
- Static network configuration

The corresponding protocol handling is implemented by the HotButton Companion module.

## LED Control

The firmware supports both the optional external WS2812 and the onboard status LED.

Available Companion controls include:

- RGB color
- Brightness
- On / Off / Toggle
- Flash programs 1–10
- LED reset

The firmware also reports the current LED state back to Companion.

### Startup State

LED settings are **not restored from previous operation**. At every normal boot, the firmware initializes a defined default state:

| Setting | Default |
| --- | --- |
| State | Off |
| Color | White (`255,255,255`) |
| Flash | Off / Solid |

During startup, the onboard LED may temporarily display network/status information.

After the startup sequence has completed, the initialized LED state is sent to Companion so that Companion feedbacks and variables match the actual HotButton state.

## Serial Debugging

Serial debug output is controlled centrally in the Arduino sketch.

Normal operation:

```cpp
static const bool debug_serial = false;
```

Enable diagnostic serial output:

```cpp
static const bool debug_serial = true;
```

When debugging is disabled, normal serial diagnostic output is suppressed.

## Companion Module

The corresponding **Bitfocus Companion HotButton module** is located in the root of this repository.

Companion handles:

- Manual and Learn pairing modes
- Pair / ACK handshake
- Device online state
- Long-press detection
- LED actions
- LED feedbacks
- Variables
- Static network configuration

The firmware and Companion module contained in this repository were developed and hardware-tested together as the **HotButton v1.0 implementation**.

## Firmware Structure

```text
HotButton_OSC_v1_0/
├── HotButton_OSC_v1_0.ino
├── WAVESHARE_DRIVER_NOTE.txt
└── src/
    ├── socket.c
    ├── socket.h
    ├── dhcp.c
    ├── dhcp.h
    ├── w6300.c
    ├── w6300.h
    └── ...
```

Keep the supplied `src` directory together with the Arduino sketch when building the firmware.
