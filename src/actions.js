const COLOR_CHOICES = [
  { id: 'red', label: 'Red' },
  { id: 'green', label: 'Green' },
  { id: 'blue', label: 'Blue' },
  { id: 'yellow', label: 'Yellow' },
  { id: 'orange', label: 'Orange' },
  { id: 'purple', label: 'Purple' },
  { id: 'white', label: 'White' },
  { id: 'custom', label: 'Custom RGB' },
]

function dynamicIntegerOption(id, label, defaultValue, min, max) {
  return {
    type: 'number',
    id,
    label,
    default: defaultValue,
    min,
    max,
    asInteger: true,
    clampValues: true,
  }
}

export function updateActions(self) {
  self.setActionDefinitions({
    led_color: {
      name: 'LED Color',
      options: [
        {
          type: 'dropdown',
          id: 'color',
          label: 'Color',
          default: 'red',
          choices: COLOR_CHOICES,
          disableAutoExpression: true,
        },
        {
          ...dynamicIntegerOption('red', 'Red', 255, 0, 255),
          isVisibleExpression: "$(options:color) == 'custom'",
        },
        {
          ...dynamicIntegerOption('green', 'Green', 0, 0, 255),
          isVisibleExpression: "$(options:color) == 'custom'",
        },
        {
          ...dynamicIntegerOption('blue', 'Blue', 0, 0, 255),
          isVisibleExpression: "$(options:color) == 'custom'",
        },
      ],
      callback: (action) => {
        if (action.options.color === 'custom') {
          self.sendOsc('/led/custom', [
            { type: 'i', value: self.clampInt(action.options.red, 0, 255) },
            { type: 'i', value: self.clampInt(action.options.green, 0, 255) },
            { type: 'i', value: self.clampInt(action.options.blue, 0, 255) },
          ])
        } else {
          self.sendOsc(`/led/${action.options.color}`, [])
        }
      },
    },
    led_brightness: {
      name: 'LED Brightness',
      options: [
        {
          type: 'dropdown',
          id: 'operation',
          label: 'Operation',
          default: 'set',
          choices: [
            { id: 'set', label: 'Set' },
            { id: 'increase', label: 'Increase' },
            { id: 'decrease', label: 'Decrease' },
          ],
          disableAutoExpression: true,
        },
        dynamicIntegerOption('value', 'Value', 10, 0, 100),
      ],
      callback: (action) => {
        const value = self.clampInt(action.options.value, 0, 100)
        const suffix = action.options.operation === 'increase' ? '/add' : action.options.operation === 'decrease' ? '/sub' : ''
        self.sendOsc(`/led/brightness${suffix}`, [{ type: 'i', value }])
      },
    },
    led_flash: {
      name: 'LED Flash',
      options: [
        {
          type: 'dropdown',
          id: 'program',
          label: 'Flash Program',
          default: 1,
          choices: Array.from({ length: 10 }, (_, i) => ({ id: i + 1, label: `Program ${i + 1}` })),
          disableAutoExpression: true,
        },
      ],
      callback: (action) => self.sendOsc('/led/flash', [{ type: 'i', value: self.clampInt(action.options.program, 1, 10) }]),
    },
    led_solid: {
      name: 'LED Solid',
      options: [],
      callback: () => self.sendOsc('/led/solid', []),
    },
    led_on: {
      name: 'LED On',
      options: [],
      callback: () => self.sendOsc('/led/on', []),
    },
    led_off: {
      name: 'LED Off',
      options: [],
      callback: () => self.sendOsc('/led/off', []),
    },
    led_reset: {
      name: 'LED Reset',
      options: [],
      callback: () => self.sendOsc('/led/reset', []),
    },
    network_configuration: {
      name: 'Network Configuration',
      options: [
        {
          type: 'dropdown',
          id: 'mode',
          label: 'Network Mode',
          default: 'dhcp',
          choices: [
            { id: 'dhcp', label: 'DHCP' },
            { id: 'static', label: 'Static' },
          ],
          disableAutoExpression: true,
        },
        {
          type: 'textinput',
          id: 'ip',
          label: 'IP Address',
          default: '10.1.10.50',
          useVariables: true,
          isVisibleExpression: "$(options:mode) == 'static'",
        },
        {
          type: 'textinput',
          id: 'subnet',
          label: 'Subnet Mask',
          default: '255.255.255.0',
          useVariables: true,
          isVisibleExpression: "$(options:mode) == 'static'",
        },
        {
          type: 'textinput',
          id: 'gateway',
          label: 'Gateway',
          default: '0.0.0.0',
          useVariables: true,
          isVisibleExpression: "$(options:mode) == 'static'",
          description: 'Use 0.0.0.0 when no gateway is required.',
        },
      ],
      callback: (action) => {
        if (action.options.mode === 'dhcp') {
          if (!self.sendOsc('/network/dhcp', [])) return
          self.enterLearnModeAfterDhcp()
          return
        }

        const ip = String(action.options.ip ?? '').trim()
        const subnet = String(action.options.subnet ?? '').trim()
        const gateway = String(action.options.gateway ?? '0.0.0.0').trim()
        if (!self.isIPv4(ip) || !self.isIPv4(subnet) || !self.isIPv4(gateway)) {
          self.log('warn', `Static network configuration rejected: invalid IPv4 value(s)`)
          return
        }
        if (!self.sendOsc('/network/static', [
          { type: 's', value: ip },
          { type: 's', value: subnet },
          { type: 's', value: gateway },
        ])) return
        self.followStaticIp(ip)
      },
    },
  })
}
