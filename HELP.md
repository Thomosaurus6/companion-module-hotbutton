# HotButton v1.0.0 --- Companion Help

HotButton is a PoE-powered physical network button for **Bitfocus
Companion**. The hardware communicates using OSC over UDP. One Companion
connection represents one physical HotButton.

## Startup connection check and LED states

A previously paired HotButton sends `/<deviceId>/connection/check` with a unique integer token to its stored Companion IP every 5 seconds, starting **after** the network-ready indication. The module replies with `/<deviceId>/connection/ack` containing the same token. The HotButton checks the sender IP and token before accepting the reply. The 60-second window does **not** delay a successful connection.

After 60 seconds without a reply, a physical press broadcasts discovery while the checks to the original Companion continue. The HotButton accepts pairing only while unpaired or in this recovery state; it ignores pairing after a successful connection for the rest of the boot session. The existing heartbeat/online timeout is independent of this startup check.

| Condition | LED indication | Duration / meaning |
|---|---|---|
| No usable IP | Red smooth breathing | Until DHCP, static IP or fallback is ready |
| Normal network ready | Green, Flash program 10 (50 ms on/off) | 3 s; connection check starts afterwards |
| DHCP fallback | Violet, Flash program 10 | 3 s; fallback IP `192.168.1.10` |
| Previously paired: startup check | Blue smooth breathing | Up to 60 s; check every 5 s |
| Unpaired or recovery after 60 s | Orange smooth breathing | Until valid ACK or successful pairing; recovery keeps checking every 5 s |
| Companion ACK or successful pairing | Green, Flash program 10 | 3 s, then Companion-controlled LEDs |
| Physical boot reset accepted | Red, Flash program 10 | 3 s; reset has priority |

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
handshake when the HotButton is unpaired or in recovery **and the physical button is pressed**.

## 2. Network configuration

### Web interface

The firmware provides a network setup page on **TCP port 80**. Open the
HotButton's current IP address in a browser.

The page shows the device name, unique device ID, active IP, subnet, gateway, network mode, pairing state and Companion IP. It allows configuration of:

-   **Network mode:** DHCP or Static
-   **IP address**
-   **Subnet mask**
-   **Gateway**
-   **Companion unicast IP**

In DHCP mode the static IP/subnet/gateway fields are disabled; in Static mode they are editable. Use **Save & Restart Network** to save the settings and restart the
network interface.

For the Companion unicast IP:

-   a valid IPv4 address stores that address but **does not pair** the device; changing an existing paired IP clears the pairing flag
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

-   red breathing while no usable IP is available
-   green Flash 10 for 3 seconds after normal network configuration
    succeeds
-   violet Flash 10 for 3 seconds when the DHCP fallback becomes active

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

## Device names

The device's OSC ID is generated from its RP2350 hardware ID. Its friendly display name (64 adjectives x 64 animals/mythical creatures) is randomly assigned on first boot and stored separately from pairing/network settings. In Edit Connection the **Device Name** field shows the friendly name; `device_name` is also available as a Companion variable. The connection label remains user-controlled. The Connections list's static module subtitle remains **HotButton: OSC** (the module API cannot dynamically replace this manifest label). The name is included in connection status messages.

After upgrading from `hotbutton_1`, re-learn/re-pair the device; the OSC Device ID has changed. Manually setting the Companion IP on the HotButton does not pair it. Pairing requires a physical button press.

## Connection module v1.2.0 — Custom Flash

The **LED Flash Custom** action sends `/hotbutton_<device-id>/led/flash/custom` with three OSC int32 arguments:

- On Duration: 10–60000 ms
- Off Duration: 10–60000 ms
- Fade Duration: 0–min(On Duration, Off Duration) ms

The v1.1.5 pairing fix is retained: changing from Learn to Manual when assigning a static IP preserves the device ID.

