export function updateVariableDefinitions(self) {
  self.setVariableDefinitions({
    device_id: { name: 'Device ID' },
    device_ip: { name: 'Device IP' },
    button_state: { name: 'Button State' },
    last_event: { name: 'Last Event' },
    pairing_state: { name: 'Pairing State' },
    online: { name: 'Device Online' },
    last_seen: { name: 'Last Seen' },

    led_state: { name: 'LED State' },
    led_color: { name: 'LED Color (R,G,B)' },
    led_brightness: { name: 'LED Brightness (%)' },
    led_flash: { name: 'LED Flash Program' },

    last_press_duration_ms: { name: 'Last Press Duration (ms)' },
    last_press_duration_seconds: { name: 'Last Press Duration (seconds)' },
    last_press_duration_minutes: { name: 'Last Press Duration (minutes)' },

    last_press_interval_ms: { name: 'Last Press-to-Press Interval (ms)' },
    last_press_interval_seconds: { name: 'Last Press-to-Press Interval (seconds)' },
    last_press_interval_minutes: { name: 'Last Press-to-Press Interval (minutes)' },
  })
}
