export function updateVariableDefinitions(self) {
  self.setVariableDefinitions({
    device_id: { name: 'Device ID' },
    device_ip: { name: 'Device IP' },
    button_state: { name: 'Button State' },
    last_event: { name: 'Last Event' },
    pairing_state: { name: 'Pairing State' },
    online: { name: 'Device Online' },
    last_seen: { name: 'Last Seen' },
    last_press_duration_ms: { name: 'Last Press Duration (ms)' },
    last_press_interval_ms: { name: 'Last Press-to-Press Interval (ms)' },
  })
}
