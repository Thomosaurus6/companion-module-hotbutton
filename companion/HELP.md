# HotButton

Companion module for the RP2350 PoE OSC HotButton.

## Connection

- **Pairing Mode — Manual:** enter the HotButton IP address. The Device ID is validated/re-learned from valid traffic received from that exact IP, including after every module start.
- **Pairing Mode — Learn:** the module waits for the next physical HotButton press and stores its source IP + Device ID.
- **Forget current pairing:** available directly in the Learn-mode connection configuration; the Companion connection itself remains intact.
- **Long Press Threshold:** default `2000 ms` and measured inside Companion.
- **Trigger Long Press while held:** enabled by default. Disable it to defer the Long Press event until release.
- **OSC Port:** defaults to `13122`.
- **Online Timeout:** defaults to `20000 ms`. Firmware heartbeat is expected every `5000 ms`.

Multiple HotButton instances can use the same UDP listen port. The module uses Companion's shared UDP socket API and filters paired devices by source IP + Device ID.

## Actions

- LED Color — Red, Green, Blue, Yellow, Orange, Purple, White, Custom RGB
- LED Brightness — Set, Increase, Decrease
- LED Flash — program 1–10
- LED Solid
- LED On
- LED Off
- LED Reset
- Network Configuration — DHCP or Static

Number action fields can use Companion's expression mode, so Custom RGB and Brightness can be driven by variables/expressions.

## Feedbacks

- **Button Press Pulse:** true for the configured hold time after each press. Release does not shorten the pulse.
- **Long Press Event:** intended primarily as a Trigger source. Its timing and release behavior are defined in the connection configuration.
- **Device Paired**
- **Device Online**

## Variables

- Device ID
- Device IP
- Button State
- Last Event
- Pairing State
- Online
- Last Seen
- Last Press Duration (ms): time from the most recent Press to its Release
- Last Press-to-Press Interval (ms): time between the two most recent Press events, independent of Release

## Heartbeat

Firmware sends `/hotbutton_X/heartbeat` every 5 seconds with one OSC integer argument:

- `0` = physical button currently released
- `1` = physical button currently pressed

The heartbeat is primarily used for online detection. The button-state value is also a safety synchronization mechanism: if a UDP Release packet is lost, a later heartbeat carrying `0` clears a stale pressed state in Companion.
