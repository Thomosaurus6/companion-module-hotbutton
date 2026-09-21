# HotButton Firmware

Firmware for the **HotButton PoE network button**, based on the
**Waveshare RP2350-POE-ETH** and designed for the matching Bitfocus
Companion module.

## Version

**HotButton Firmware v1.0**

Arduino sketch: `HotButton_OSC_v1_0.ino`

## Hardware

  Component         Configuration
  ----------------- --------------------------
  Controller        Waveshare RP2350-POE-ETH
  MCU               RP2350A
  Ethernet          WIZnet W6300
  Power             PoE
  Button            GPIO0 → GND
  External WS2812   GPIO1
  Onboard WS2812    GPIO25

The external WS2812 is optional.

## Features

-   DHCP and persistent static IPv4 configuration
-   Temporary DHCP fallback at `192.168.1.10/24`
-   HTTP network setup interface on port 80
-   Configurable Companion unicast IP through the web interface
-   Persistent Companion pairing
-   Broadcast discovery while unpaired
-   Unicast OSC after pairing
-   Physical Press and Release events
-   5-second heartbeat
-   Companion-controlled RGB LED
-   LED brightness, flash and state synchronization
-   Physical network/pairing recovery
-   Optional serial debugging

Long-press detection is handled by Companion, not by the firmware.

## Ethernet / W6300 driver

The firmware uses the Waveshare/WIZnet W6300 sources included in the
local `src` directory.

> **Important:** The included `socket.c` contains a required correction
> for non-blocking socket operation. Do not replace the driver directory
> with an unmodified upstream copy without reviewing this fix.

## Network behavior

### DHCP

Without a stored static configuration, the HotButton starts in DHCP
mode.

If no DHCP lease is obtained after **60 seconds**, the firmware
activates a temporary fallback:

-   IP: `192.168.1.10`
-   Subnet: `255.255.255.0`
-   Gateway: `0.0.0.0`

The fallback is not written to flash. A later reboot therefore starts
with DHCP again unless a static configuration has been deliberately
stored.

### Fallback setup

When the fallback becomes active, the onboard status LED blinks **orange
for 7 seconds**.

Connect a computer to the same network and assign it an address in
`192.168.1.0/24`, for example:

-   Computer: `192.168.1.20`
-   Subnet: `255.255.255.0`

Then open:

`http://192.168.1.10`

Use the web interface to configure the final network settings.

### Static IP

A static IPv4 configuration can be stored through either the Companion
network configuration workflow or the web interface.

Supported settings:

-   IP address
-   Subnet mask
-   Gateway

## Web network setup

A small HTTP server runs on **TCP port 80** whenever the HotButton has a
usable IP.

Open the current HotButton IP in a browser. The page displays:

-   current IP
-   subnet
-   gateway
-   network mode
-   current Companion IP / broadcast state

The page allows editing:

-   DHCP or Static mode
-   HotButton IP address
-   subnet mask
-   gateway
-   Companion unicast IP

Press **Save & Restart Network** to store the selected settings and
restart the network interface.

For Companion IP:

-   enter the desired IPv4 address to store it as the unicast
    destination
-   enter `0.0.0.0` to clear the stored Companion IP and return OSC
    transmission to broadcast discovery

There is currently no mDNS hostname such as `hotbutton.local`; access
the interface by IP address.

## Pairing and OSC routing

While unpaired, outgoing OSC discovery/button traffic uses directed
broadcast.

After a successful Companion pair/ACK handshake, the HotButton stores
the Companion source IP and sends normal traffic by **unicast** to that
address. Pairing survives normal power cycles.

The web interface can also directly set or clear the stored Companion
unicast IP.

Default OSC UDP port: **13122**.

## Physical recovery

Recovery is available only during the first **30 seconds after boot**.

Hold the physical button for **5 seconds**. The firmware clears:

-   stored static network configuration
-   stored Companion pairing/IP

Successful reset recognition is indicated by **fast red blinking for 3
seconds**. The button may then be released.

The HotButton returns to DHCP and broadcast/unpaired operation. If DHCP
remains unavailable, the 60-second fallback is used.

## Status LED

During startup/network acquisition:

-   **Red blinking:** no usable IP
-   **Green blinking for 7 seconds:** normal network configuration ready
-   **Orange blinking for 7 seconds:** fallback `192.168.1.10` active
-   **Fast red blinking for 3 seconds:** physical recovery accepted

After the status indication, the LED returns to normal user-controlled
behavior.

## User LED defaults

LED settings are not restored from previous operation. At boot:

  Setting   Default
  --------- -----------------------
  State     Off
  Color     White (`255,255,255`)
  Flash     Off / Solid

The initialized state is reported to Companion after startup.

## Serial debugging

Normal operation:

``` cpp
static const bool debug_serial = false;
```

For diagnostics:

``` cpp
static const bool debug_serial = true;
```

Serial speed: **115200 baud**.

## Firmware structure

``` text
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

Keep the supplied `src` directory together with the Arduino sketch.
