# HotButton v1.0.0 — Companion Help

HotButton is a PoE-powered physical network button for **Bitfocus Companion**. The hardware communicates using OSC over UDP. One Companion connection represents one physical HotButton.

This document describes the stable v1.0.0 behavior of the Companion module and matching HotButton firmware.

## 1. Connection and pairing

### Pairing Mode: Learn

No HotButton IP is required initially. Press the physical button to discover the HotButton. A heartbeat alone cannot claim an unpaired Learn connection.

After discovery, Companion sends:

```text
/<deviceId>/pair
```

The HotButton stores the **source IP** of this request as its Companion IP and replies by unicast:

```text
/<deviceId>/pair/ack
```

Companion shows `PAIRING` while waiting for the ACK and `PAIRED` only after a valid ACK from the expected HotButton IP and Device ID. Pair requests are retried once per second, up to five attempts. Failure does not erase existing pairing data; a later valid device contact can start a new attempt.

After pairing, normal HotButton traffic is sent by unicast to the stored Companion IP. The stored Companion IP survives normal HotButton reboots and power cycles.

**Forget current pairing and wait for next button press** clears only Companion's local learned IP/Device-ID state. It intentionally does not send a pairing-clear command to the HotButton.

### Pairing Mode: Manual

Enter the HotButton IP address. Companion accepts device traffic from that IP, learns/validates its Device ID and completes the same pair/ACK handshake.

The Device ID is revalidated after a Companion module restart and when the Manual IP changes.

### Learn to Static IP

After a Learn-mode device is successfully paired, static-network fields appear in the connection configuration:

- IP Address
- Subnet Mask — default `255.255.255.0`
- Gateway — default `0.0.0.0`

Saving sends the network settings once to the currently learned DHCP IP. Companion then switches the connection to Manual mode using the new static HotButton IP and relearns/validates the device there.

A gateway of `0.0.0.0` disables gateway monitoring in the firmware. It does not disable OSC heartbeat traffic.

### Physical DHCP / pairing recovery

True device-side unpairing is performed physically. During the first **30 seconds after boot**, hold the physical button continuously for **5 seconds**. The firmware clears:

- stored static network configuration
- stored Companion pairing/IP

The HotButton returns to DHCP and unpaired/broadcast discovery mode. The held button is suppressed until release to avoid a phantom Press. After the first 30 seconds, this recovery gesture is unavailable until the next reboot.

## 2. Communication

Default OSC UDP port: `13122`.

Before pairing, discovery traffic uses directed broadcast. After pair/ACK, normal Press, Release, heartbeat and LED-state traffic use unicast to the stored Companion IP.

Multiple HotButtons and multiple Companion HotButton connections can use the same OSC port. Connections validate traffic by source IP and Device ID as appropriate to the current pairing state.

## 3. Button events

Firmware messages:

```text
/<deviceId>_press
/<deviceId>_release
```

A normal Press is processed immediately. Release completes the physical press cycle.

### Long Press

Long Press is calculated entirely in Companion. Connection options include:

- Long Press Threshold — default `2000 ms`, maximum `60000 ms`
- Trigger Long Press while button is still held — default enabled

A Long Press does not replace Press. A held button generates Press first and can later qualify as Long Press.

### Timing variables

Companion records the last completed press duration and the interval between consecutive Press events in three representations:

- milliseconds
- seconds with two decimal places
- `MM:SS.hh`

Minutes are not limited to 59.

## 4. Heartbeat and online state

Firmware heartbeat:

```text
/<deviceId>/heartbeat <state>
```

where `0 = released` and `1 = pressed`.

The firmware normally sends a heartbeat every 5 seconds. Companion's default Online Timeout is 20000 ms and is configurable from 5000 to 120000 ms.

Heartbeat behavior:

- valid traffic refreshes online state
- `heartbeat = 0` may correct a stale pressed state if a UDP Release was lost
- `heartbeat = 1` never creates a synthetic Press
- when the device times out, runtime button/long-press state is cleared; timing history remains available

Companion status output is only updated when the status/message actually changes, avoiding a `Status: ok` log line for every Press, Release or heartbeat.

## 5. LED Actions

### LED Color

Predefined colors:

- Red
- Green
- Blue
- Yellow
- Orange
- Purple
- White
- Custom RGB

Custom RGB accepts independent integer values from 0–255 for Red, Green and Blue.

Firmware command for Custom RGB:

```text
/<deviceId>/led/custom <R> <G> <B>
```

Changing color does not automatically turn the LED on.

### LED Brightness

Operations: Set, Increase, Decrease. Range: 0–100 %. Values are clamped by the firmware.

```text
/<deviceId>/led/brightness <0-100>
/<deviceId>/led/brightness/add <0-100>
/<deviceId>/led/brightness/sub <0-100>
```

Brightness changes do not automatically turn the LED on.

### LED Flash

Selections: Off, 1–10.

- Off = solid output
- Program 1 = slowest
- Program 10 = fastest

```text
/<deviceId>/led/solid
/<deviceId>/led/flash <1-10>
```

Flashing is generated locally by the firmware, not by continuous Companion traffic.

### LED State

Operations: On, Off, Toggle.

```text
/<deviceId>/led/on
/<deviceId>/led/off
/<deviceId>/led/toggle
```

Off hides the visible output but does not erase the current color, brightness or flash selection. On uses the currently stored configuration.

### LED Reset

```text
/<deviceId>/led/reset
```

Reset state:

- Color: White
- Brightness: 0 %
- Flash: Off / Solid
- Power: Off

The firmware also starts the user LED in its defined default/reset state after boot; it does not restore the previous user LED configuration.

## 6. LED state synchronization

Companion can request a complete LED snapshot:

```text
/<deviceId>/led/state/get
```

The firmware reports the current LED state after changes and during synchronization. Companion uses these reports to update LED variables and feedbacks.

State reports:

```text
/<deviceId>/led/state              <0|1>
/<deviceId>/led/color              <R> <G> <B>
/<deviceId>/led/brightness/state   <0-100>
/<deviceId>/led/flash/state        <0-10>
```

Flash value `0` means Off/Solid. On startup, the firmware reports its initialized/default LED state so Companion does not retain stale LED feedback values from before the reboot.

## 7. Feedbacks

### Button Press Pulse

True for a configurable period after each physical Press. Default: 1000 ms. Release does not shorten the pulse.

### Long Press Event

Represents the configured Long Press event. Depending on configuration it becomes active while held or pulses after release.

### Device Online

True while valid HotButton traffic is being received within the configured Online Timeout.

### LED State

Boolean feedback that is true while the reported user LED power state is On.

### LED Color

Advanced feedback that dynamically sets the Companion button background to the RGB color currently reported by the HotButton.

## 8. Variables

| Variable | Meaning |
| --- | --- |
| `device_id` | Paired/validated HotButton Device ID |
| `device_ip` | Current HotButton IP |
| `button_state` | `pressed`, `released`, or empty when unknown/offline |
| `last_event` | Most recently processed button event |
| `pairing_state` | Current pairing state |
| `online` | Current online state |
| `last_seen` | Timestamp of most recent valid device traffic |
| `led_state` | `On` / `Off` |
| `led_color` | Reported RGB as `R,G,B` |
| `led_brightness` | Reported brightness, 0–100 |
| `led_flash` | `Off` or program `1`–`10` |
| `last_press_duration_ms` | Last completed press duration in ms |
| `last_press_duration_seconds` | Duration in seconds, two decimals |
| `last_press_duration_minutes` | Duration as `MM:SS.hh` |
| `last_press_interval_ms` | Press-to-Press interval in ms |
| `last_press_interval_seconds` | Interval in seconds, two decimals |
| `last_press_interval_minutes` | Interval as `MM:SS.hh` |

## 9. Pairing OSC messages

```text
/<deviceId>/pair
/<deviceId>/pair/ack
```

The firmware may support a device-side pairing-clear command for maintenance, but the Companion v1.0 workflow intentionally does not use it. Device-side pairing is cleared with the physical boot recovery procedure.

## 10. Network configuration OSC

Static network configuration uses three OSC string arguments:

```text
/<deviceId>/network/static <IP> <Subnet> <Gateway>
```

The Companion module sends this only as part of the Learn-to-Static workflow.

## 11. Hardware reference

Current reference design:

- Waveshare RP2350-POE-ETH
- RP2350A + W6300 Ethernet
- PoE
- GPIO0 momentary button to GND using pull-up
- optional 24 × WS2812 ring on GPIO1 through TXS0108E
- onboard WS2812 on GPIO25

The external LED ring can be omitted. The firmware has a software master-brightness ceiling in addition to the user-facing 0–100 % brightness value.

## 12. Development and packaging

Requirements:

- Node.js 22
- Yarn 4

Install and package:

```bash
yarn install
yarn package
```

Expected v1.0.0 package output:

```text
kra55k0pf-hotbutton-1.0.0.tgz
```

Example macOS development location:

```text
~/Library/Application Support/companion/modules/HotButton_OSC
```

## 13. v1.0.0 release status

v1.0.0 is the first stable, end-to-end hardware-tested release. Tested workflows include Learn and Manual pairing, pair/ACK, persistent unicast operation, HotButton and Companion restarts, Learn-to-Static configuration, physical recovery, Press/Release delivery, LED presets, Custom RGB, brightness, flash programs, LED state control and LED feedback/variable synchronization.

## License

MIT

Maintainer: Thomas Thielen / Thomosaurus6
