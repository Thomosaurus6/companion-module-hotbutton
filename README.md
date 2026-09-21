# HotButton

**PoE-powered physical OSC button for Bitfocus Companion**

HotButton is a network-connected physical button based on the Waveshare
RP2350-POE-ETH. One Ethernet cable provides power and communication. The
matching Companion module handles pairing, button events, long-press
logic, online monitoring, LED control, feedbacks and variables.

> **Release:** v1.0.0 --- stable hardware-tested release

## Highlights

-   PoE-powered RP2350/W6300 hardware
-   OSC over UDP, default port `13122`
-   One Companion connection per physical HotButton
-   Manual or Learn pairing with pair/ACK handshake
-   Persistent Companion pairing and unicast operation after pairing
-   DHCP and persistent static IPv4 configuration
-   Built-in network setup web interface on TCP port `80`
-   DHCP fallback to `192.168.1.10/24` after 60 seconds without a lease
-   Physical boot recovery to DHCP/unpaired mode
-   Immediate Press and Release events
-   Companion-side configurable Long Press detection
-   5 s heartbeat with configurable online timeout
-   RGB LED color, brightness, flash, state and feedback synchronization
-   Multiple HotButtons can share the same OSC port

## Pairing

### Learn Mode

Create the Companion connection in **Learn** mode and press the physical
HotButton. The Press is used to discover its source IP and Device ID.
Companion then sends `/<deviceId>/pair`; the HotButton stores the source
IP of that request as its Companion IP and replies with
`/<deviceId>/pair/ack`.

After the ACK, the HotButton sends Press, Release, heartbeat and
LED-state traffic by **unicast** to the stored Companion IP. Pairing
survives normal HotButton power cycles.

### Manual Mode

Enter the HotButton IP address. Companion learns/validates the Device ID
from that IP and completes the same pair/ACK handshake.

## Network setup web interface

The firmware contains a small HTTP configuration interface on **port
80**. Open the current HotButton IP in a browser, for example:

`http://192.168.1.10`

The page displays the active network configuration and allows
configuration of:

-   Network mode: DHCP or Static
-   HotButton IP address
-   Subnet mask
-   Gateway
-   Companion unicast IP

Choose **Save & Restart Network** to store the settings and restart the
HotButton network interface.

Setting the Companion unicast IP to `0.0.0.0` clears the stored
Companion IP and returns OSC transmission to broadcast discovery.
Entering a Companion IP stores that address as the unicast destination.

The web interface is available on the HotButton's active IP in normal
DHCP, static-IP and fallback operation. There is currently no mDNS
hostname such as `hotbutton.local`; use the IP address directly.

## DHCP fallback

When the HotButton is configured for DHCP and has not obtained a lease
after **60 seconds**, it activates a temporary fallback configuration:

-   IP: `192.168.1.10`
-   Subnet: `255.255.255.0`
-   Gateway: `0.0.0.0`

The fallback is runtime-only and is not stored in flash. On the next
boot, DHCP is attempted again.

To configure a HotButton in fallback mode, put the computer in the same
`192.168.1.0/24` network, for example `192.168.1.20 / 255.255.255.0`,
open `http://192.168.1.10`, configure the desired network settings and
use **Save & Restart Network**.

Status indication:

-   Red blinking: no usable IP yet
-   Green blinking for 7 seconds: normal network configuration is ready
-   Orange blinking for 7 seconds: DHCP fallback `192.168.1.10` is
    active

## Physical recovery

During the first **30 seconds after boot**, hold the physical button
continuously for **5 seconds**. The HotButton clears:

-   the stored static network configuration
-   the stored Companion pairing/IP

The reset is acknowledged by **fast red blinking for 3 seconds** so the
button can be released immediately after the reset has been recognized.

The HotButton then returns to DHCP and unpaired/broadcast discovery
mode. If no DHCP server is available, the 60-second fallback applies.
The held button is suppressed until release so recovery does not create
a phantom Press.

## Button behavior

A physical Press is reported immediately and Release separately. Long
Press detection runs entirely in Companion; the default threshold is
2000 ms and can be changed without reflashing the firmware.

The heartbeat is primarily used for online monitoring and safety
synchronization. `heartbeat = 0` can correct a missed Release;
`heartbeat = 1` does not synthesize a new Press.

## LED control

Available controls include:

-   LED Color --- presets and Custom RGB
-   LED Brightness --- Set, Increase, Decrease
-   LED Flash --- Off/Solid or programs 1--10
-   LED State --- On, Off, Toggle
-   LED Reset

At boot the user LED starts in its defined default/reset state instead
of restoring the previous user state. The firmware reports its current
LED state to Companion so feedbacks and variables synchronize correctly.

## Documentation

Detailed Companion connection settings, actions, feedbacks, variables
and protocol behavior are in [`companion/HELP.md`](companion/HELP.md).

Firmware-specific documentation is in
[`RP2350-POE-ETH-Firmware/HotButton_OSC_v1_0/README.md`](RP2350-POE-ETH-Firmware/HotButton_OSC_v1_0/README.md).

## Hardware

-   Waveshare RP2350-POE-ETH
-   RP2350A
-   WIZnet W6300
-   PoE
-   Momentary push button on GPIO0
-   Optional 24 × WS2812 ring on GPIO1 through a TXS0108E level shifter
-   Onboard WS2812 on GPIO25

The external LED ring may be omitted.

## Development

Requirements: Node.js 22 and Yarn 4.

``` bash
yarn install
yarn package
```

A successful package build creates `kra55k0pf-hotbutton-1.0.0.tgz`.

## Important W6300 driver note

The firmware directory contains the tested Waveshare/WIZnet driver
sources. Its `socket.c` includes a required correction for non-blocking
socket operation. Do not replace it blindly with an unmodified upstream
copy.

## License

MIT

Maintainer: Thomas Thielen / Thomosaurus6
