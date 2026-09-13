import { combineRgb } from '@companion-module/base'

export function updateFeedbacks(self) {
  self.setFeedbackDefinitions({
    button_press_pulse: {
      name: 'Button Press Pulse',
      type: 'boolean',
      defaultStyle: {
        bgcolor: combineRgb(255, 0, 0),
        color: combineRgb(255, 255, 255),
      },
      options: [
        {
          type: 'number',
          id: 'holdMs',
          label: 'Hold Time (ms)',
          default: 1000,
          min: 0,
          max: 3600000,
          step: 100,
          asInteger: true,
          clampValues: true,
          description: 'Feedback remains true for this long after each press event. Release does not shorten it.',
        },
      ],
      callback: (feedback) => {
        const holdMs = Math.max(0, Number(feedback.options.holdMs ?? 1000))
        return self.lastPressAt > 0 && Date.now() - self.lastPressAt < holdMs
      },
    },
    button_long_press: {
      name: 'Long Press Event',
      description: 'Use this Boolean feedback as a Companion Trigger source for the configured Long Press event.',
      type: 'boolean',
      defaultStyle: {
        bgcolor: combineRgb(255, 128, 0),
        color: combineRgb(255, 255, 255),
      },
      options: [],
      callback: () => self.longPressActive,
    },
    device_paired: {
      name: 'Device Paired',
      type: 'boolean',
      defaultStyle: {
        bgcolor: combineRgb(0, 160, 0),
        color: combineRgb(255, 255, 255),
      },
      options: [],
      callback: () => self.isPaired(),
    },
    device_online: {
      name: 'Device Online',
      type: 'boolean',
      defaultStyle: {
        bgcolor: combineRgb(0, 160, 0),
        color: combineRgb(255, 255, 255),
      },
      options: [],
      callback: () => self.online,
    },
  })
}
