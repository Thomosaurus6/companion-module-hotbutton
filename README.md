# HotButton

**PoE-powered physical OSC button for Bitfocus Companion**

HotButton is a network-connected physical button built around the Waveshare RP2350-POE-ETH. One Ethernet cable provides power and communication. A custom Companion module handles pairing, button events, long-press logic, online monitoring, LED control, feedbacks and variables.

> **Release:** v1.0.0 — first stable hardware-tested release

## Highlights

- PoE-powered RP2350/W6300 hardware
- OSC over UDP, default port `13122`
- One Companion connection per physical HotButton
- Manual or Learn pairing with explicit pair/ACK handshake
- Persistent Companion pairing and unicast operation after pairing
- Learn-to-Static network setup from the Companion connection
- Physical boot recovery to DHCP/unpaired mode
- Immediate Press and Release events
- Companion-side configurable Long Press detection
- Press-duration and press-to-press interval variables
- 5 s heartbeat with configurable online timeout
- LED Color presets and Custom RGB
- LED Brightness 0–100 %, Set/Increase/Decrease
- LED Flash Off/Solid plus programs 1–10
- LED On/Off/Toggle and Reset
- LED State and dynamic LED Color feedbacks
- LED state, color, brightness and flash variables
- Multiple HotButtons can share the same OSC port

## Pairing

### Learn Mode

Create the Companion connection in **Learn** mode and press the physical HotButton. The Press is used to discover its source IP and Device ID. Companion then sends `/<deviceId>/pair`; the HotButton stores the source IP of that request as its Companion IP and replies with `/<deviceId>/pair/ack`.

After the ACK, the HotButton sends Press, Release, heartbeat and LED-state traffic by **unicast** to the stored Companion IP. Pairing survives normal HotButton power cycles.

The connection setting **Forget current pairing and wait for next button press** only clears Companion's local learned device information. It does not erase the Companion IP stored in the HotButton.

### Manual Mode

Enter the HotButton IP address. Companion learns/validates the Device ID from that IP and completes the same pair/ACK handshake.

### Learn to Static IP

After successful Learn pairing, enter the desired IP, subnet and gateway in the connection settings and save. Companion sends the network configuration once to the current DHCP address and switches the connection to Manual mode using the new IP.

Defaults:

- Subnet: `255.255.255.0`
- Gateway: `0.0.0.0`

Gateway `0.0.0.0` disables gateway monitoring in the firmware; it does **not** disable the OSC heartbeat.

### Physical Recovery

During the first **30 seconds after boot**, hold the physical button continuously for **5 seconds** to clear the stored static network configuration and stored Companion pairing. The HotButton returns to DHCP and unpaired/broadcast discovery mode. The held button is suppressed until release so the recovery action does not create a phantom Press.

## Button behavior

A physical Press is reported immediately. Release is reported separately. Long Press detection runs entirely in Companion; the default threshold is 2000 ms and can be changed without reflashing the firmware.

A Long Press does not replace the normal Press: the Press occurs first, then the Long Press event can qualify later.

The heartbeat is primarily used for online monitoring and safety synchronization. `heartbeat = 0` can correct a missed Release; `heartbeat = 1` does not synthesize a new Press.

## LED control

The LED configuration consists of power state, stored RGB color, brightness and flash program. Color, brightness and flash changes do not automatically turn the LED on.

**Actions:**

- LED Color — Red, Green, Blue, Yellow, Orange, Purple, White, Custom RGB
- LED Brightness — Set, Increase, Decrease; 0–100 %
- LED Flash — Off/Solid or program 1–10; 1 slowest, 10 fastest
- LED State — On, Off, Toggle
- LED Reset — White, 0 % brightness, Flash Off/Solid, Power Off

At boot the user LED starts in its defined reset/default state rather than restoring the previous user state. The firmware reports its current LED state to Companion so feedbacks and variables synchronize correctly.

**Feedbacks:**

- Button Press Pulse
- Long Press Event
- Device Online
- LED State
- LED Color — dynamically uses the reported RGB value as the Companion button background

**LED variables:** `led_state`, `led_color`, `led_brightness`, `led_flash`.

## Documentation

Detailed connection settings, actions, feedbacks, variables, OSC messages, recovery behavior and development notes are in [`companion/HELP.md`](companion/HELP.md).

## Hardware

Current reference hardware:

- Waveshare RP2350-POE-ETH
- RP2350A
- W6300 Ethernet controller
- PoE
- Momentary push button on GPIO0
- Optional 24 × WS2812 LED ring on GPIO1 through a TXS0108E level shifter
- Onboard WS2812 status/user LED on GPIO25

The external LED ring may be omitted; the onboard LED remains usable.

## Development

Requirements: Node.js 22 and Yarn 4.

```bash
yarn install
yarn package
```

A successful package build creates `kra55k0pf-hotbutton-1.0.0.tgz`.

## v1.0.0

First stable release after end-to-end hardware testing. The release includes persistent unicast pairing, DHCP/static setup and recovery, button/long-press handling, heartbeat/online state, complete LED control, LED feedback synchronization, timing variables and reduced Companion status-log noise.

## License

MIT

Maintainer: Thomas Thielen / Thomosaurus6
