# HotButton – OSC PoE Button for Bitfocus Companion
HotButton is a network-connected physical button designed for use with **Bitfocus Companion**.
The hardware communicates with Companion using **OSC over UDP** and is designed to operate from a single Ethernet connection using **Power over Ethernet (PoE)**.
The Companion module provides automatic device pairing, button events, long-press detection, online monitoring, LED control, timing variables, and network configuration.
> **Development status:** v0.2.0  
> The Companion module and firmware are currently under active development. Full end-to-end hardware testing is still in progress.
---
# Overview
A HotButton consists of:
- Waveshare RP2350-POE-ETH controller
- RP2350 microcontroller
- W6300 Ethernet controller
- Power over Ethernet
- Physical momentary push button
- 24-pixel WS2812 RGB LED ring
- OSC/UDP communication
The general communication path is:
```text
Physical Button
      │
      ▼
HotButton Firmware
      │
      │ OSC / UDP
      ▼
Bitfocus Companion
      │
      ├── Actions
      ├── Feedbacks
      ├── Triggers
      └── Variables

One Companion connection represents one physical HotButton.

Multiple HotButtons can operate on the same network and use the same OSC port.

⸻

Companion Module

Connection Configuration

Pairing Mode

Two pairing modes are available:

Manual

The IP address of the HotButton is entered manually.

Companion listens for valid HotButton traffic from that exact IP and automatically learns the Device ID.

The stored Device ID is deliberately revalidated when the module starts. This prevents an old Device ID from remaining associated with an IP address that may later have been assigned to another device.

Changing the configured IP address also clears the learned Device ID and starts validation again.

Learn

Learn mode allows a HotButton to be paired without manually entering its IP address.

After creating the connection:

1. Select Learn.
2. Save the connection.
3. Press the physical HotButton.
4. The first valid HotButton press pairs the connection with that device.

Both the source IP address and Device ID are stored.

A heartbeat by itself will not claim an unpaired Companion connection. A physical button press is required. This prevents an arbitrary HotButton already present on the network from automatically taking over a newly created connection.

⸻

Forget Current Pairing

The connection configuration contains:

Forget current pairing and wait for next button press

When enabled and the connection is saved, Companion clears the current Learn-mode pairing and waits for the next physical HotButton press.

The option automatically resets after the pairing has been cleared.

⸻

Device Information

The connection configuration displays the current state of the HotButton connection.

Pairing State

Possible states include:

WAITING FOR BUTTON
WAITING FOR IP
WAITING FOR DEVICE ID
PAIRED

Device

Displays the learned HotButton Device ID.

Example:

hotbutton_1

Active IP

Displays the IP address currently associated with the HotButton.

Device Status

Displays whether the device is currently considered:

ONLINE
OFFLINE

⸻

Static Network Configuration

Static network configuration is integrated directly into the Learn Mode pairing workflow.

It is not a separate Companion Action.

After a HotButton has been paired in Learn Mode, the connection configuration provides:

IP Address
Subnet Mask
Gateway

Default values:

Subnet Mask: 255.255.255.0
Gateway:     0.0.0.0

To configure a new HotButton:

1. Select Learn Mode.
2. Press the physical HotButton.
3. Companion learns its current DHCP/source IP and Device ID.
4. Enter the desired static IP address.
5. Adjust Subnet Mask and Gateway if required.
6. Save the connection.

Companion sends the static network configuration to the HotButton using its currently learned address.

The HotButton stores the new configuration persistently.

Companion then automatically changes the connection to:

Pairing Mode: Manual
Manual IP:    <new static IP>

The previously learned Device ID is cleared.

Once traffic is received from the HotButton at its new static IP, Companion automatically validates and learns the Device ID again.

This prevents a stale Device ID from remaining associated with an IP address.

A gateway of:

0.0.0.0

disables gateway ping monitoring in the HotButton firmware.

This does not disable OSC heartbeat transmission or Companion online detection.

⸻

Hardware Network Recovery

The firmware provides a physical recovery mechanism for network configuration.

Holding the physical button during startup for approximately:

10 seconds

clears the stored static network configuration and returns the HotButton to DHCP.

This allows a device with an unknown or incorrect static IP configuration to be recovered without requiring network access.

⸻

Button Events

The HotButton firmware sends separate OSC messages for button presses and releases.

Default device example:

/hotbutton_1_press
/hotbutton_1_release

Companion handles all higher-level button timing.

⸻

Press

A physical button press immediately generates a normal Press event.

A press does not wait to determine whether the user is performing a long press.

This means a long press is:

Press
+
Long Press

and not a mutually exclusive alternative to a normal press.

This behavior makes normal button operation immediate and avoids adding artificial latency to every press.

⸻

Long Press

Long-press detection is performed entirely inside Companion.

The HotButton firmware does not need to know the configured long-press duration.

This allows the timing to be changed independently for each Companion connection without modifying or reflashing the HotButton firmware.

Long Press Threshold

Default:

2000 ms

The threshold can be configured in the connection settings.

⸻

Trigger Long Press While Button Is Still Held

Two long-press behaviors are supported.

Enabled

When the button remains pressed for the configured threshold:

PRESS
      │
      ├──── threshold reached
      │
      ▼
LONG PRESS
      │
      │
      ▼
RELEASE

The Long Press event fires immediately while the physical button is still being held.

Disabled

The button must first exceed the configured threshold.

The Long Press event is then generated when the button is released.

PRESS
      │
      ├──── threshold reached
      │
      │
      ▼
RELEASE
      │
      ▼
LONG PRESS

⸻

Press Timing

Companion records two timing measurements:

* Last Press Duration
* Last Press-to-Press Interval

Each measurement is available in three representations.

⸻

Last Press Duration

The duration of the most recently completed:

Press → Release

cycle.

Milliseconds

last_press_duration_ms

Raw integer value in milliseconds.

Example:

65234

Seconds

last_press_duration_seconds

Seconds rounded to two decimal places.

Example:

65.23

Minutes

last_press_duration_minutes

Human-readable format:

MM:SS.hh

where hh represents hundredths of a second.

Example:

01:05.23

⸻

Last Press-to-Press Interval

Contains the time between the two most recent Press events.

This measurement is independent of the Release event.

Example:

Press
   │
   │ 1250 ms
   ▼
Press

The same value is available as:

last_press_interval_ms       = 1250
last_press_interval_seconds  = 1.25
last_press_interval_minutes  = 00:01.25

This can be useful for building custom multi-press or timing logic inside Companion.

For calculations and expressions, the raw millisecond variable provides the highest precision.

⸻

Heartbeat and Online Detection

The HotButton firmware periodically sends a heartbeat.

Default interval:

5000 ms

The heartbeat is used for two purposes:

1. Device online detection
2. Button-state synchronization

The heartbeat includes the current debounced physical button state.

Example:

/hotbutton_1/heartbeat 0

means:

Button released

and:

/hotbutton_1/heartbeat 1

means:

Button pressed

⸻

Lost UDP Release Protection

OSC communication uses UDP.

UDP intentionally does not guarantee packet delivery.

For example, Companion could receive:

PRESS

but, in an unusual network-loss situation, fail to receive:

RELEASE

Without additional synchronization, Companion could incorrectly believe that the button is still being held.

The heartbeat provides a safety mechanism.

If Companion believes the button is pressed but subsequently receives:

heartbeat = 0

the internal button state is corrected to released.

The heartbeat is therefore a safety synchronization mechanism and not the primary method of detecting normal button presses.

Normal Press and Release messages remain responsible for immediate button response.

⸻

Online Timeout

Default:

20000 ms

If no valid HotButton traffic is received within the configured timeout, the device is considered offline.

With the default firmware heartbeat interval of 5000 ms, the default timeout allows several missed heartbeat packets before declaring the device offline.

The timeout can be changed in the connection configuration.

⸻

OSC Port

Default:

13122

Multiple Companion HotButton connections can listen on the same OSC port.

Each connection filters incoming traffic using the paired source IP address and Device ID.

⸻

Actions

The Companion module provides actions for controlling the HotButton LED.

⸻

LED Color

Sets the stored LED color.

Available predefined colors:

* Red
* Green
* Blue
* Yellow
* Orange
* Purple
* White
* Custom RGB

Changing the color does not automatically turn the LED output on.

This allows LED parameters to be prepared before enabling the output.

⸻

Custom RGB

Custom RGB allows independent control of:

Red:   0–255
Green: 0–255
Blue:  0–255

The fields support Companion expressions and variables.

Example:

R = 255
G = 120
B = 0

⸻

LED Brightness

Brightness is controlled using a single action with three operations:

Set
Increase
Decrease

Values use a range of:

0–100 %

Examples:

Set 50
Increase 10
Decrease 20

The firmware clamps the resulting brightness to the valid 0–100 % range.

Brightness changes do not automatically turn the LED output on.

⸻

LED Flash

The LED Flash action selects the LED operating mode.

Available selections are:

Off
1
2
3
4
5
6
7
8
9
10

Off disables flashing and returns the LED to solid output.

Programs 1–10 provide progressively faster flash rates:

Program 1  = slowest
Program 10 = fastest

Flash timing is performed by the HotButton firmware itself.

This avoids requiring continuous OSC traffic from Companion to generate LED flashing.

Changing the flash mode does not automatically turn the LED output on.

⸻

LED State

The LED State action controls the visible LED output.

Available operations:

On
Off
Toggle

On

Turns the LED output on using the currently stored configuration.

For example:

Color:      Orange
Brightness: 40 %
Mode:       Flash 3

If a flash mode is active, the firmware begins with the illuminated phase.

Off

Turns the visible LED output off.

LED Off does not erase:

* color
* brightness
* flash program
* solid/flash mode

A later LED On restores the previously configured state.

Toggle

Changes the current LED power state:

On  → Off
Off → On

The stored color, brightness, and flash configuration remain unchanged.

⸻

LED Reset

LED Reset provides a defined zero/panic state.

It resets the user LED configuration to:

Color:      White
Brightness: 0 %
Flash:      Off
Power:      Off

⸻

Feedbacks

The module provides the following Companion feedbacks.

Button Press Pulse

Becomes true whenever a Press event is received.

Default pulse duration:

1000 ms

The pulse duration is configurable.

A Release event does not shorten the pulse.

This makes the feedback particularly useful as a Companion Trigger source.

⸻

Long Press Event

Represents the Long Press event.

When Trigger Long Press while button is still held is enabled, the feedback becomes true when the threshold is reached and remains active until Release.

When long press is configured to trigger on Release, the feedback generates an event pulse after a qualifying long press is released.

⸻

Device Online

True while the paired HotButton is considered online.

This feedback can be used to indicate whether Companion is currently receiving valid traffic from the HotButton.

Pairing itself remains available through the pairing_state variable and connection status display rather than as a separate button feedback.

⸻

Variables

The module exposes the following Companion variables.

Variable	Description
device_id	Device ID of the paired HotButton
device_ip	Current HotButton IP address
button_state	Current Companion button state
last_event	Most recently received/processed button event
pairing_state	Current pairing state
online	Current online state
last_seen	Timestamp of the most recent valid device communication
last_press_duration_ms	Last completed press duration in milliseconds
last_press_duration_seconds	Last completed press duration in seconds, rounded to two decimal places
last_press_duration_minutes	Last completed press duration formatted as MM:SS.hh
last_press_interval_ms	Time between the two most recent Press events in milliseconds
last_press_interval_seconds	Press-to-Press interval in seconds, rounded to two decimal places
last_press_interval_minutes	Press-to-Press interval formatted as MM:SS.hh

⸻

OSC Interface

The default OSC UDP port is:

13122

The Device ID forms part of the OSC address for device-specific commands.

Default example Device ID:

hotbutton_1

⸻

Button Messages

/hotbutton_1_press
/hotbutton_1_release

⸻

Heartbeat

/hotbutton_1/heartbeat <button-state>

Button state:

0 = released
1 = pressed

⸻

LED Control

The firmware supports OSC commands for:

* predefined colors
* custom RGB
* brightness
* brightness increase/decrease
* flash programs
* solid mode
* LED on
* LED off
* LED toggle
* LED reset

Custom RGB:

/hotbutton_1/led/custom <R> <G> <B>

Brightness:

/hotbutton_1/led/brightness <0-100>

Brightness increase:

/hotbutton_1/led/brightness/add <0-100>

Brightness decrease:

/hotbutton_1/led/brightness/sub <0-100>

Flash:

/hotbutton_1/led/flash <1-10>

Solid / Flash Off:

/hotbutton_1/led/solid

LED On:

/hotbutton_1/led/on

LED Off:

/hotbutton_1/led/off

LED Toggle:

/hotbutton_1/led/toggle

LED Reset:

/hotbutton_1/led/reset

⸻

Network Control

Static network configuration:

/hotbutton_1/network/static

with three OSC string arguments:

IP
Subnet
Gateway

The Companion module uses this command during the Learn-to-Static network configuration workflow.

⸻

LED Hardware

The current HotButton design uses:

24 × WS2812 RGB LEDs

The external LED ring is controlled by the RP2350 through a level shifter.

The firmware currently applies a master brightness ceiling to reduce maximum LED output.

This master limit is separate from the user-configurable 0–100 % LED brightness setting.

⸻

Development

Requirements

Current development environment:

Node.js 22
Yarn 4

The module currently targets the Companion Node 22 runtime.

⸻

Install Dependencies

From the module directory:

yarn install

⸻

Build / Validate Package

yarn package

A successful build creates a compressed Companion module package.

Generated build artifacts and dependencies are intentionally excluded from Git.

⸻

Local Companion Development

The repository can be placed inside a Companion Developer Modules directory.

Example on macOS:

~/Library/Application Support/companion/modules/HotButton_OSC

Configure Companion’s Developer Modules path to point to the parent directory:

~/Library/Application Support/companion/modules

Companion can then load the HotButton module directly from the development source.

⸻

Current Development Status

Current module version:

0.2.0

Implemented functionality includes:

* Manual pairing
* Learn pairing
* Pairing reset
* Device ID learning and validation
* Learn-to-Static network configuration
* Online/offline monitoring
* Press handling
* Release handling
* Companion-side Long Press detection
* Press duration measurement
* Press interval measurement
* Millisecond, second, and minute timing variables
* Heartbeat synchronization
* LED color control
* Custom RGB
* Brightness control
* Flash programs
* Solid / Flash Off mode
* LED On / Off / Toggle
* LED Reset
* Companion feedbacks
* Companion variables
* Multiple HotButton connections using the shared OSC port

Full end-to-end testing with the current firmware and physical HotButton hardware is still in progress.

⸻

Project Structure

companion-module-hotbutton/
├── companion/
│   ├── manifest.json
│   └── HELP.md
├── src/
│   ├── actions.js
│   ├── config.js
│   ├── feedbacks.js
│   ├── main.js
│   └── variables.js
├── package.json
├── yarn.lock
└── README.md

⸻

License

MIT

⸻

Maintainer

Thomas Thielen
GitHub: Thomosaurus6