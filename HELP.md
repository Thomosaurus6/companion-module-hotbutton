# HotButton v1.0.0 --- Companion Help

HotButton is a PoE-powered physical network button for **Bitfocus
Companion**. The hardware communicates using OSC over UDP. One Companion
connection represents one physical HotButton.

## 1. Connection and pairing

### Pairing Mode: Learn

No HotButton IP is required initially. Press the physical button to
discover the HotButton. A heartbeat alone cannot claim an unpaired Learn
connection.

After discovery, Companion sends:

``` text
/<deviceId>/pair
```

The HotButton stores the **source IP** of this request as its Companion
IP and replies by unicast:

``` text
/<deviceId>/pair/ack
```

Companion shows `PAIRING` while waiting for the ACK and `PAIRED` only
after a valid ACK from the expected HotButton IP and Device ID.

After pairing, normal HotButton traffic is sent by unicast to the stored
Companion IP. The stored Companion IP survives normal HotButton reboots
and power cycles.

**Forget current pairing and wait for next button press** clears
Companion's local learned IP/Device-ID state. Device-side pairing can
also be cleared through the web interface by saving Companion IP
`0.0.0.0`, or through physical recovery.

### Pairing Mode: Manual

Enter the HotButton IP address. Companion accepts device traffic from
that IP, learns/validates its Device ID and completes the same pair/ACK
handshake.

## 2. Network configuration

### Web interface

The firmware provides a network setup page on **TCP port 80**. Open the
HotButton's current IP address in a browser.

The page shows the active IP, subnet, gateway, network mode and
Companion IP. It allows configuration of:

-   **Network mode:** DHCP or Static
-   **IP address**
-   **Subnet mask**
-   **Gateway**
-   **Companion unicast IP**

Use **Save & Restart Network** to save the settings and restart the
network interface.

For the Companion unicast IP:

-   a valid IPv4 address stores that address as the Companion unicast
    destination
-   `0.0.0.0` clears the stored Companion IP and returns OSC
    transmission to broadcast discovery

The web interface is available whenever the HotButton has a usable IP
address. Access is by IP address; `hotbutton.local`/mDNS is not
implemented.

### DHCP fallback

When DHCP mode is active and no lease is obtained after **60 seconds**,
the firmware activates:

-   IP Address: `192.168.1.10`
-   Subnet Mask: `255.255.255.0`
-   Gateway: `0.0.0.0`

This fallback is temporary and is not stored in flash. On the next boot
DHCP is attempted again.

For recovery/configuration without a DHCP server, configure the computer
for the same subnet, for example `192.168.1.20 / 255.255.255.0`, then
open:

`http://192.168.1.10`

Configure the required network mode/settings and press **Save & Restart
Network**.

Status indication:

-   red blinking while no usable IP is available
-   green blinking for 7 seconds after normal network configuration
    succeeds
-   orange blinking for 7 seconds when the DHCP fallback becomes active

### Companion Learn-to-Static workflow

The existing Companion network configuration workflow remains available.
After a Learn-mode device is paired, the connection can send a static
IP, subnet and gateway to the HotButton and then switch the Companion
connection to Manual mode.

Gateway `0.0.0.0` is valid and does not disable OSC heartbeat traffic.

## 3. Physical recovery

Recovery is available during the first **30 seconds after boot**.

Hold the physical button continuously for **5 seconds**. The firmware
clears:

-   stored static network configuration
-   stored Companion pairing/IP

Successful recognition of the reset is indicated by **fast red blinking
for 3 seconds**. The button can then be released.

The HotButton returns to DHCP and unpaired/broadcast discovery mode. If
DHCP is unavailable, the normal 60-second fallback to `192.168.1.10/24`
applies. The held button is suppressed until release so the recovery
action does not create a phantom Press.

## 4. Button behavior

### Press

A physical Press is sent immediately.

``` text
/<deviceId>_press
```

### Release

Release is sent separately.

``` text
/<deviceId>_release
```

### Long Press

Long Press detection is performed entirely in Companion. The default
threshold is **2000 ms**. A Long Press does not replace the normal
Press; the Press occurs immediately and the Long Press event can qualify
later.

### Heartbeat

The HotButton sends a heartbeat every **5 seconds**.

``` text
/<deviceId>/heartbeat <0|1>
```

`0` means released and may correct a missed Release. `1` means
physically held but does not synthesize a new Press.

## 5. LED actions

### LED Color

Presets plus Custom RGB.

### LED Brightness

Set, Increase and Decrease, range 0--100%.

### LED Flash

Off/Solid plus programs 1--10, where 1 is slowest and 10 fastest.

### LED State

On, Off and Toggle.

### LED Reset

Reset state:

-   Color: White
-   Brightness: 0%
-   Flash: Off/Solid
-   Power: Off

Color, brightness and flash changes do not automatically turn the LED
on.

## 6. LED state synchronization

Companion can request:

``` text
/<deviceId>/led/state/get
```

The firmware reports:

``` text
/<deviceId>/led/state              <0|1>
/<deviceId>/led/color              <R> <G> <B>
/<deviceId>/led/brightness/state   <0-100>
/<deviceId>/led/flash/state        <0-10>
```

The initialized/default LED state is also synchronized after startup.

## 7. Feedbacks

-   **Button Press Pulse**
-   **Long Press Event**
-   **Device Online**
-   **LED State**
-   **LED Color** --- uses the reported RGB value as the Companion
    button background

## 8. Variables

  -----------------------------------------------------------------------
  Variable                            Meaning
  ----------------------------------- -----------------------------------
  `device_id`                         Paired/validated HotButton Device
                                      ID

  `device_ip`                         Current HotButton IP

  `button_state`                      `pressed`, `released`, or empty
                                      when unknown/offline

  `last_event`                        Most recently processed button
                                      event

  `pairing_state`                     Current pairing state

  `online`                            Current online state

  `last_seen`                         Timestamp of most recent valid
                                      device traffic

  `led_state`                         `On` / `Off`

  `led_color`                         Reported RGB as `R,G,B`

  `led_brightness`                    Reported brightness, 0--100

  `led_flash`                         `Off` or program `1`--`10`

  `last_press_duration_ms`            Last completed press duration in ms

  `last_press_duration_seconds`       Duration in seconds

  `last_press_duration_minutes`       Duration as `MM:SS.hh`

  `last_press_interval_ms`            Press-to-Press interval in ms

  `last_press_interval_seconds`       Interval in seconds

  `last_press_interval_minutes`       Interval as `MM:SS.hh`
  -----------------------------------------------------------------------

## 9. Pairing OSC messages

``` text
/<deviceId>/pair
/<deviceId>/pair/ack
```

Before pairing, outgoing button/heartbeat traffic uses directed
broadcast. After pairing, it uses the stored Companion IP as the unicast
destination.

## 10. Network configuration OSC

Static network configuration uses three OSC string arguments:

``` text
/<deviceId>/network/static <IP> <Subnet> <Gateway>
```

The web interface is an additional configuration path and does not
replace the existing Companion protocol.

## 11. Default ports

-   OSC UDP: `13122`
-   HTTP network setup: `80`
