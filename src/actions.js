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

const FLASH_CHOICES = [
  { id: 0, label: 'Off' },
  ...Array.from({ length: 10 }, (_, i) => ({
    id: i + 1,
    label: `${i + 1} ${'▶'.repeat(i + 1)}`,
  })),
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
        const suffix =
          action.options.operation === 'increase'
            ? '/add'
            : action.options.operation === 'decrease'
              ? '/sub'
              : ''

        self.sendOsc(`/led/brightness${suffix}`, [{ type: 'i', value }])
      },
    },

    led_flash: {
      name: 'LED Flash',
      description: 'Off = solid output. Program 1 = slowest, Program 10 = fastest.',
      options: [
        {
          type: 'dropdown',
          id: 'program',
          label: 'Flash Program',
          default: 0,
          choices: FLASH_CHOICES,
          disableAutoExpression: true,
        },
      ],
      callback: (action) => {
        const program = self.clampInt(action.options.program, 0, 10)

        if (program === 0) {
          self.sendOsc('/led/solid', [])
        } else {
          self.sendOsc('/led/flash', [{ type: 'i', value: program }])
        }
      },
    },

    led_state: {
      name: 'LED State',
      options: [
        {
          type: 'dropdown',
          id: 'state',
          label: 'State',
          default: 'on',
          choices: [
            { id: 'on', label: 'On' },
            { id: 'off', label: 'Off' },
            { id: 'toggle', label: 'Toggle' },
          ],
          disableAutoExpression: true,
        },
      ],
      callback: (action) => self.sendOsc(`/led/${action.options.state}`, []),
    },

    led_reset: {
      name: 'LED Reset',
      description: 'Resets the LED to White, 0% brightness, Flash Off and LED Off.',
      options: [],
      callback: () => self.sendOsc('/led/reset', []),
    },
  })
}
