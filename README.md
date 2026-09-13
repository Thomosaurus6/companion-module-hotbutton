# HotButton

**PoE-powered physical OSC button for Bitfocus Companion**

HotButton is a network-connected physical button designed for **Bitfocus Companion**.  
It combines a physical push button, a 24-pixel RGB LED ring and Ethernet/PoE in a standalone device that communicates with Companion using **OSC over UDP**.

One Ethernet cable provides both **power and communication**.

> **Status:** v0.2.0 – Development  
> The module is functional and currently undergoing end-to-end hardware testing.

---

## Features

- PoE-powered standalone hardware
- OSC over UDP
- Manual IP configuration or automatic **Learn Mode**
- Multiple HotButtons on the same network
- Immediate Press and Release events
- Companion-side configurable Long Press detection
- Press duration and press interval measurement
- Heartbeat-based online monitoring and state synchronization
- 24-pixel RGB LED ring control
- Predefined colors and Custom RGB
- 0–100 % brightness control
- 10 flash programs
- Solid / On / Off / Reset LED control
- Static IP and DHCP configuration directly from Companion
- Companion Actions, Feedbacks, Triggers and Variables

---

## How it works

```text
┌─────────────────────┐
│      HotButton      │
│                     │
│  Physical Button    │
│  RGB LED Ring       │
│  RP2350 + Ethernet  │
└──────────┬──────────┘
           │
           │  PoE / Ethernet
           │  OSC over UDP
           ▼
┌─────────────────────┐
│ Bitfocus Companion  │
│                     │
│ Actions             │
│ Feedbacks           │
│ Triggers            │
│ Variables           │
└─────────────────────┘
```

Each Companion connection represents one physical HotButton.

The default OSC port is:

```text
13122
```

---

## Pairing

HotButton supports two connection modes.

### Manual

Enter the IP address of the HotButton manually. Companion automatically learns and validates the Device ID from that device.

### Learn

No IP address is required.

Create the connection in **Learn Mode** and press the physical HotButton. Companion automatically pairs the connection with the source IP and Device ID of that button.

A heartbeat alone cannot claim an unpaired connection – pairing requires an intentional physical button press.

---

## Button Events

The physical button provides:

- **Press**
- **Release**
- **Long Press**

A normal Press event is generated immediately.

Long Press detection is handled entirely inside Companion, so the threshold can be configured without changing or reflashing the HotButton firmware.

Companion also provides:

- Last press duration
- Time between consecutive presses

---

## LED Control

The integrated RGB LED ring can be controlled directly from Companion.

Available controls include:

- Red
- Green
- Blue
- Yellow
- Orange
- Purple
- White
- Custom RGB
- Brightness Set / Increase / Decrease
- Flash programs 1–10
- Solid mode
- LED On
- LED Off
- LED Reset

Color, brightness, effect and power state are handled independently, allowing LED states to be prepared before the output is switched on.

---

## Network Management

Network configuration can also be changed from Companion.

Supported modes:

- **DHCP**
- **Static IP**

Static configuration includes:

- IP address
- Subnet mask
- Gateway

A physical recovery function is available: holding the HotButton during startup for approximately 10 seconds clears the static configuration and returns the device to DHCP.

---

## Companion Integration

The module provides dedicated:

- **Actions** for LED and network control
- **Feedbacks** for button presses, Long Press, pairing and online status
- **Variables** for device information, button state and timing data

For a complete description of all connection settings, Actions, Feedbacks, Variables and OSC commands, see:

**`companion/HELP.md`**

---

## Hardware

Current HotButton hardware is based on:

- Waveshare RP2350-POE-ETH
- RP2350 microcontroller
- W6300 Ethernet controller
- Power over Ethernet
- Physical momentary push button
- 24 × WS2812 RGB LED ring

---

## Development

Requirements:

```text
Node.js 22
Yarn 4
```

Install dependencies:

```bash
yarn install
```

Validate and build the Companion module:

```bash
yarn package
```

The module can be loaded directly through Companion's **Developer Modules** functionality during development.

---

## Project Status

Current version:

**v0.2.0**

Implemented:

- Pairing and Learn Mode
- Press / Release / Long Press
- Heartbeat and online detection
- LED control
- Network configuration
- Feedbacks and Variables
- Multiple HotButton instances

Full end-to-end testing with the current HotButton firmware and physical hardware is in progress.

---

## License

MIT

## Maintainer

**Thomas Thielen**  
GitHub: **Thomosaurus6**