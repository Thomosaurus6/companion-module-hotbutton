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
- Learn-to-Static network setup directly from the connection configuration
- Multiple HotButtons on the same network and OSC port
- Immediate Press and Release events
- Companion-side configurable Long Press detection
- Press duration and press interval measurement
- Heartbeat-based online monitoring and state synchronization
- 24-pixel RGB LED ring control
- Predefined colors and Custom RGB
- 0–100 % brightness control
- 10 flash programs plus solid output
- LED On / Off / Toggle
- LED Reset
- Companion Actions, Feedbacks and Variables
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
│ Variables           │
└─────────────────────┘

Each Companion connection represents one physical HotButton.

The default OSC port is:

13122

⸻

Pairing

HotButton supports two connection modes.

Manual

Enter the IP address of the HotButton manually. Companion automatically learns and validates the Device ID from traffic received from that IP.

The Device ID is revalidated when the module starts or the configured Manual IP changes.

Learn

No IP address is required.

Create the connection in Learn Mode and press the physical HotButton. Companion automatically pairs the connection with the source IP and Device ID of that button.

A heartbeat alone cannot claim an unpaired connection. Pairing requires an intentional physical button press.

The current Learn pairing can be forgotten from the connection configuration to wait for the next physical button press.

Learn to Static IP

After a device has been paired in Learn Mode, an optional static network configuration becomes available directly in the connection settings.

Configure:

* IP address
* Subnet mask
* Gateway

Saving the configuration sends the static network settings once to the HotButton at its currently learned DHCP address.

Companion then automatically switches the connection to Manual Mode using the new static IP address. The Device ID is cleared and learned again from traffic received at the new address.

The default subnet mask is:

255.255.255.0

The default gateway is:

0.0.0.0

A gateway of 0.0.0.0 disables gateway ping monitoring in the HotButton firmware. The normal OSC heartbeat remains active.

To return a HotButton to DHCP, hold the physical button during startup for approximately 10 seconds. This clears the stored static network configuration.

⸻

Button Events

The physical button provides:

* Press
* Release
* Long Press

A normal Press event is generated immediately.

Long Press detection is handled entirely inside Companion, so the threshold and trigger behavior can be configured without changing or reflashing the HotButton firmware.

A Long Press does not replace the normal Press event. A held button therefore generates a normal Press first and can subsequently qualify as a Long Press.

Companion also provides timing information for:

* Last completed press duration
* Time between consecutive Press events

Timing values are available as:

* Milliseconds
* Seconds with two decimal places
* Minutes in MM:SS.hh format

⸻

Heartbeat and Online Detection

The HotButton periodically sends an OSC heartbeat containing the current button state.

The heartbeat is used for:

* Device online detection
* Button state synchronization
* Recovery if a UDP Release message is lost

Normal Press and Release messages remain the primary source for button events.

A heartbeat reporting a released button can correct a stale pressed state in Companion.

A heartbeat reporting a pressed button confirms the current state but does not generate a new Press event.

⸻

LED Control

The 24-pixel RGB LED ring can be controlled directly from Companion.

Available Actions:

LED Color

* Predefined colors
* Custom RGB

Changing the color does not automatically switch the LED on.

LED Brightness

* Set
* Increase
* Decrease
* Range: 0–100 %

Changing brightness does not automatically switch the LED on.

LED Flash

Available modes:

* Off
* Program 1
* Program 2
* Program 3
* Program 4
* Program 5
* Program 6
* Program 7
* Program 8
* Program 9
* Program 10

Off selects solid output.

Program 1 is the slowest flash program and Program 10 is the fastest.

Changing the flash program does not automatically switch the LED on.

LED State

* On
* Off
* Toggle

LED Off only disables the visible output. The configured color, brightness and flash program remain stored.

LED On restores the stored state. If a flash program is active, it starts with the ON phase.

LED Reset

Reset returns the LED to:

* White
* 0 % brightness
* Flash Off / Solid
* Power Off

⸻

Feedbacks

The module provides three Companion Feedbacks:

* Button Press Pulse
* Long Press Event
* Device Online

Button Press Pulse

Generates a configurable pulse on every physical Press event.

The pulse is independent of the Release event and is not shortened if the button is released before the configured pulse duration expires.

Long Press Event

Indicates a qualified Long Press.

Depending on the connection configuration, the Long Press can trigger while the button is still held or when the button is released.

Device Online

True while the HotButton is considered online based on valid device traffic and the configured Online Timeout.

⸻

Variables

The module provides variables for device information, button state, pairing, online status and timing.

Current variables include:

device_id
device_ip
button_state
last_event
pairing_state
online
last_seen
last_press_duration_ms
last_press_duration_seconds
last_press_duration_minutes
last_press_interval_ms
last_press_interval_seconds
last_press_interval_minutes

The _ms variables contain integer milliseconds.

The _seconds variables contain seconds with two decimal places.

The _minutes variables use:

MM:SS.hh

Minutes are not limited to 59.

⸻

Companion Integration

The module provides dedicated:

* Actions for LED control
* Feedbacks for Press, Long Press and online status
* Variables for device information, button state, pairing and timing data
* Connection configuration for Manual/Learn pairing, Long Press behavior, communication settings and Learn-to-Static network setup

For a complete description of all connection settings, Actions, Feedbacks, Variables and OSC commands, see:

companion/HELP.md

⸻

Hardware

Current HotButton hardware is based on:

* Waveshare RP2350-POE-ETH
* RP2350A microcontroller
* W6300 Ethernet controller
* Power over Ethernet
* Physical momentary push button
* 24 × WS2812 RGB LED ring
* TXS0108E level shifter for the LED data signal

Communication with Companion uses OSC over UDP.

⸻

Development

Requirements:

Node.js 22
Yarn 4

Install dependencies:

yarn install

Validate and build the Companion module:

yarn package

The module can be loaded directly through Companion’s Developer Modules functionality during development.

Repository:

https://github.com/Thomosaurus6/companion-module-hotbutton

Issues:

https://github.com/Thomosaurus6/companion-module-hotbutton/issues

⸻

Project Status

Current version:

v0.2.0 – Development

Implemented:

* Manual and Learn pairing
* Learn-to-Static network setup
* Press / Release
* Companion-side Long Press detection
* Press duration and interval timing
* Heartbeat and online detection
* LED Color
* LED Brightness
* LED Flash
* LED State On / Off / Toggle
* LED Reset
* Feedbacks and Variables
* Multiple HotButton instances on the same network and OSC port

Full end-to-end testing with the current HotButton firmware and physical hardware is in progress.

⸻

License

MIT

Maintainer

Thomas Thielen
GitHub: Thomosaurus6