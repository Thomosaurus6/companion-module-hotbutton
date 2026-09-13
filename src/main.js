import { InstanceBase, InstanceStatus } from '@companion-module/base'
import { getConfigFields } from './config.js'
import { updateActions } from './actions.js'
import { updateFeedbacks } from './feedbacks.js'
import { updateVariableDefinitions } from './variables.js'

const DEFAULT_PORT = 13122
const DEFAULT_TIMEOUT = 20000
const DEFAULT_LONG_PRESS_MS = 2000
const RELEASE_LONG_PRESS_PULSE_MS = 1000

class HotButtonInstance extends InstanceBase {
  constructor(internal) {
    super(internal)
    this.config = {}
    this.socket = null
    this.socketPort = null
    this.deviceId = ''

    this.online = false
    this.lastSeenAt = 0
    this.lastEvent = ''

    this.buttonState = ''
    this.lastPressAt = 0
    this.pressStartedAt = 0
    this.previousPressAt = 0
    this.lastPressDurationMs = 0
    this.lastPressIntervalMs = 0

    this.longPressQualified = false
    this.longPressActive = false
    this.longPressTimer = null
    this.longPressPulseTimer = null

    this.statusTimer = null
    this.feedbackTimer = null
  }

  async init(config, _isFirstInit, _secrets) {
    this.config = this.normalizeConfig(config)

    // Manual mode intentionally validates/re-learns the Device ID from the
    // configured IP every time the module starts. This prevents a stale ID
    // from remaining attached to an IP that now belongs to another HotButton.
    if (this.config.pairingMode === 'manual' && this.config.deviceId) {
      this.config.deviceId = ''
      this.saveConfig({ ...this.config })
    }

    this.deviceId = this.config.deviceId || ''

    updateVariableDefinitions(this)
    updateActions(this)
    updateFeedbacks(this)

    this.updateVariables()
    this.restartSocket()
    this.startTimers()
    this.refreshStatus()
  }

  async destroy() {
    this.stopTimers()
    this.clearLongPressTimers()
    this.closeSocket()
  }

  async configUpdated(config, _secrets) {
    const old = this.config
    const next = this.normalizeConfig(config)

    const staticNetworkRequested =
      old.pairingMode === 'learn' &&
      next.pairingMode === 'learn' &&
      Boolean(old.learnedIp) &&
      Boolean(old.deviceId || this.deviceId) &&
      Boolean(next.staticIp)

    if (staticNetworkRequested) {
      const targetIp = old.learnedIp
      const deviceId = old.deviceId || this.deviceId
      const staticIp = next.staticIp
      const subnet = next.staticSubnet || '255.255.255.0'
      const gateway = next.staticGateway || '0.0.0.0'

      if (!this.isIPv4(staticIp) || !this.isIPv4(subnet) || !this.isIPv4(gateway)) {
        this.log('error', 'Static network configuration contains an invalid IPv4 address.')
      } else {
        const port = Number(old.oscPort || DEFAULT_PORT)
        const path = `/${deviceId}/network/static`

        this.oscSend(targetIp, port, path, [
          { type: 's', value: staticIp },
          { type: 's', value: subnet },
          { type: 's', value: gateway },
        ])

        next.pairingMode = 'manual'
        next.manualIp = staticIp
        next.learnedIp = ''
        next.deviceId = ''
        next.relearn = false

        next.staticIp = ''
        next.staticSubnet = '255.255.255.0'
        next.staticGateway = '0.0.0.0'

        this.deviceId = ''
        this.resetRuntimeDeviceState()

        this.saveConfig({ ...next })

        this.log(
          'info',
          `Static network configuration sent to ${deviceId} at ${targetIp}. Connection switched to Manual mode at ${staticIp}.`,
        )
      }
    }

    const manualIpChanged =
      old.pairingMode === 'manual' &&
      next.pairingMode === 'manual' &&
      old.manualIp !== next.manualIp

    const switchedToManual =
      !staticNetworkRequested &&
      old.pairingMode !== 'manual' &&
      next.pairingMode === 'manual'

    const switchedToLearn =
      old.pairingMode !== 'learn' &&
      next.pairingMode === 'learn'

    if (manualIpChanged || switchedToManual) {
      // A Manual IP always gets its Device ID validated again from traffic
      // coming from that exact IP.
      next.deviceId = ''
      next.learnedIp = ''
      this.deviceId = ''
      this.resetRuntimeDeviceState()
    }

    if (switchedToLearn) {
      next.learnedIp = ''
      next.deviceId = ''
      this.deviceId = ''
      this.resetRuntimeDeviceState()
    }

    if (next.pairingMode === 'learn' && next.relearn) {
      next.learnedIp = ''
      next.deviceId = ''
      next.relearn = false

      this.deviceId = ''
      this.resetRuntimeDeviceState()

      this.log('info', 'Pairing cleared. Waiting for next physical HotButton press.')
      this.saveConfig({ ...next })
    }

    const portChanged =
      Number(old.oscPort || DEFAULT_PORT) !==
      Number(next.oscPort || DEFAULT_PORT)

    const longPressSettingsChanged =
      Number(old.longPressThreshold || DEFAULT_LONG_PRESS_MS) !==
        Number(next.longPressThreshold || DEFAULT_LONG_PRESS_MS) ||
      Boolean(old.longPressWhileHeld ?? true) !==
        Boolean(next.longPressWhileHeld ?? true)

    this.config = next
    this.deviceId = next.deviceId || this.deviceId || ''

    if (longPressSettingsChanged && this.buttonState === 'pressed') {
      // Re-arm from the original press timestamp with the new threshold.
      this.armLongPressTimer()
    }

    if (portChanged) {
      this.restartSocket()
    }

    this.updateVariables()
    this.checkFeedbacks(
      'device_online',
      'button_press_pulse',
      'button_long_press',
    )
    this.refreshStatus()
  }

  getConfigFields() {
    return getConfigFields(this)
  }

  normalizeConfig(config = {}) {
    return {
      pairingMode: config.pairingMode === 'manual' ? 'manual' : 'learn',

      manualIp: String(config.manualIp || '').trim(),

      staticIp: String(config.staticIp || '').trim(),
      staticSubnet: String(config.staticSubnet || '255.255.255.0').trim(),
      staticGateway: String(config.staticGateway || '0.0.0.0').trim(),

      learnedIp: String(config.learnedIp || '').trim(),
      deviceId: String(config.deviceId || '').trim(),
      relearn: Boolean(config.relearn),

      oscPort: Number(config.oscPort || DEFAULT_PORT),
      onlineTimeout: Number(config.onlineTimeout || DEFAULT_TIMEOUT),

      longPressThreshold: Math.max(
        0,
        Number(config.longPressThreshold ?? DEFAULT_LONG_PRESS_MS),
      ),

      longPressWhileHeld: config.longPressWhileHeld !== false,
    }
  }

  resetRuntimeDeviceState() {
    this.online = false
    this.lastSeenAt = 0

    this.buttonState = ''
    this.pressStartedAt = 0

    this.longPressQualified = false
    this.longPressActive = false

    this.clearLongPressTimers()
  }

  getTargetIp() {
    return this.config.pairingMode === 'manual'
      ? this.config.manualIp
      : this.config.learnedIp
  }

  isPaired() {
    return Boolean(
      this.getTargetIp() &&
        (this.deviceId || this.config.deviceId),
    )
  }

  getPairingStateLabel() {
    if (this.config?.pairingMode === 'learn') {
      return this.isPaired()
        ? 'PAIRED'
        : 'WAITING FOR BUTTON'
    }

    if (!this.config?.manualIp) {
      return 'WAITING FOR IP'
    }

    return this.deviceId
      ? 'PAIRED'
      : 'WAITING FOR DEVICE ID'
  }

  getPairingStateVariable() {
    if (this.config?.pairingMode === 'learn') {
      return this.isPaired()
        ? 'paired'
        : 'waiting_for_button'
    }

    if (!this.config?.manualIp) {
      return 'waiting_for_ip'
    }

    return this.deviceId
      ? 'paired'
      : 'waiting_for_device_id'
  }

  formatSeconds(ms) {
    return (Math.max(0, Number(ms) || 0) / 1000).toFixed(2)
  }

  formatMinutes(ms) {
    const totalHundredths = Math.round(Math.max(0, Number(ms) || 0) / 10)
    const minutes = Math.floor(totalHundredths / 6000)
    const seconds = Math.floor((totalHundredths % 6000) / 100)
    const hundredths = totalHundredths % 100

    return `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}.${String(hundredths).padStart(2, '0')}`
  }

  updateVariables(lastEvent) {
    if (lastEvent) {
      this.lastEvent = lastEvent
    }

    this.setVariableValues({
      device_id:
        this.deviceId ||
        this.config.deviceId ||
        '',

      device_ip:
        this.getTargetIp() ||
        '',

      button_state:
        this.online
          ? this.buttonState
          : '',

      last_event:
        this.lastEvent ||
        '',

      pairing_state:
        this.getPairingStateVariable(),

      online:
        this.online,

      last_seen:
        this.lastSeenAt
          ? new Date(this.lastSeenAt).toISOString()
          : '',

      last_press_duration_ms:
        this.lastPressDurationMs,

      last_press_duration_seconds:
        this.formatSeconds(this.lastPressDurationMs),

      last_press_duration_minutes:
        this.formatMinutes(this.lastPressDurationMs),

      last_press_interval_ms:
        this.lastPressIntervalMs,

      last_press_interval_seconds:
        this.formatSeconds(this.lastPressIntervalMs),

      last_press_interval_minutes:
        this.formatMinutes(this.lastPressIntervalMs),
    })
  }

  restartSocket() {
    this.closeSocket()

    const port = Number(
      this.config.oscPort ||
      DEFAULT_PORT,
    )

    this.socketPort = port

    this.socket = this.createSharedUdpSocket(
      'udp4',
      (message, rinfo) =>
        this.handleUdpMessage(message, rinfo),
    )

    this.socket.on('error', (err) => {
      this.log(
        'error',
        `UDP listener error: ${err.message}`,
      )

      this.updateStatus(
        InstanceStatus.ConnectionFailure,
        `UDP ${port}: ${err.message}`,
      )
    })

    this.socket.bind(
      port,
      undefined,
      () =>
        this.log(
          'info',
          `Listening for HotButton OSC on UDP ${port}`,
        ),
    )
  }

  closeSocket() {
    if (!this.socket) {
      return
    }

    try {
      this.socket.close()
    } catch (_) {
      // A shared socket can still be binding while config changes.
      // Companion will release the pending handle with the instance
      // if necessary.
    }

    this.socket = null
    this.socketPort = null
  }

  startTimers() {
    this.stopTimers()

    this.statusTimer = setInterval(
      () => this.checkOnlineTimeout(),
      1000,
    )

    // Button Press Pulse hold-times are configured per placed feedback.
    this.feedbackTimer = setInterval(() => {
      if (this.lastPressAt) {
        this.checkFeedbacks(
          'button_press_pulse',
        )
      }
    }, 100)
  }

  stopTimers() {
    if (this.statusTimer) {
      clearInterval(this.statusTimer)
    }

    if (this.feedbackTimer) {
      clearInterval(this.feedbackTimer)
    }

    this.statusTimer = null
    this.feedbackTimer = null
  }

  clearLongPressTimers() {
    if (this.longPressTimer) {
      clearTimeout(this.longPressTimer)
    }

    if (this.longPressPulseTimer) {
      clearTimeout(this.longPressPulseTimer)
    }

    this.longPressTimer = null
    this.longPressPulseTimer = null
  }

  checkOnlineTimeout() {
    const timeout = Math.max(
      1000,
      Number(
        this.config.onlineTimeout ||
        DEFAULT_TIMEOUT,
      ),
    )

    const shouldBeOnline =
      this.isPaired() &&
      this.lastSeenAt > 0 &&
      Date.now() - this.lastSeenAt <= timeout

    if (shouldBeOnline !== this.online) {
      this.online = shouldBeOnline

      if (!this.online) {
        this.buttonState = ''
        this.pressStartedAt = 0
        this.longPressQualified = false
        this.longPressActive = false
        this.clearLongPressTimers()
      }

      this.updateVariables()
      this.checkFeedbacks(
        'device_online',
        'button_long_press',
      )
      this.refreshStatus()
    }
  }

  refreshStatus() {
    if (
      this.config.pairingMode === 'manual' &&
      !this.config.manualIp
    ) {
      this.updateStatus(
        InstanceStatus.BadConfig,
        'Manual mode: enter HotButton IP address',
      )
      return
    }

    if (!this.isPaired()) {
      this.updateStatus(
        InstanceStatus.Connecting,
        this.getPairingStateLabel(),
      )
      return
    }

    if (this.online) {
      this.updateStatus(
        InstanceStatus.Ok,
        `${this.deviceId} @ ${this.getTargetIp()}`,
      )
    } else {
      this.updateStatus(
        InstanceStatus.Disconnected,
        `${this.deviceId} @ ${this.getTargetIp()} — heartbeat timeout`,
      )
    }
  }

  parseOscMessage(buffer) {
    if (
      !Buffer.isBuffer(buffer) ||
      buffer.length < 4 ||
      buffer[0] !== 0x2f
    ) {
      return null
    }

    const readPaddedString = (start) => {
      const zero =
        buffer.indexOf(
          0,
          start,
        )

      if (zero < start) {
        return null
      }

      const text =
        buffer
          .subarray(
            start,
            zero,
          )
          .toString('utf8')

      const consumed =
        zero -
        start +
        1

      const next =
        start +
        ((consumed + 3) & ~3)

      if (next > buffer.length) {
        return null
      }

      return {
        text,
        next,
      }
    }

    const addressField =
      readPaddedString(0)

    if (
      !addressField ||
      !addressField.text.startsWith('/')
    ) {
      return null
    }

    const typeField =
      readPaddedString(
        addressField.next,
      )

    if (
      !typeField ||
      !typeField.text.startsWith(',')
    ) {
      return null
    }

    const args = []
    let pos =
      typeField.next

    for (
      const type
      of typeField.text.slice(1)
    ) {
      if (type === 'i') {
        if (
          pos + 4 >
          buffer.length
        ) {
          return null
        }

        args.push({
          type: 'i',
          value:
            buffer.readInt32BE(
              pos,
            ),
        })

        pos += 4
      } else {
        // Inbound HotButton traffic currently only needs
        // integer heartbeat args.
        return null
      }
    }

    return {
      address:
        addressField.text,
      args,
    }
  }

  parseHotButtonAddress(address) {
    let match =
      address.match(
        /^\/(hotbutton_[^/]+)_(press|release)$/,
      )

    if (match) {
      return {
        deviceId:
          match[1],
        event:
          match[2],
      }
    }

    match =
      address.match(
        /^\/(hotbutton_[^/]+)\/heartbeat$/,
      )

    if (match) {
      return {
        deviceId:
          match[1],
        event:
          'heartbeat',
      }
    }

    return null
  }

  handleUdpMessage(message, rinfo) {
    const osc =
      this.parseOscMessage(
        message,
      )

    if (!osc) {
      return
    }

    const parsed =
      this.parseHotButtonAddress(
        osc.address,
      )

    if (!parsed) {
      return
    }

    const sourceIp =
      rinfo.address

    if (
      this.config.pairingMode === 'learn' &&
      !this.isPaired()
    ) {
      // Intentional physical pairing:
      // heartbeat/release alone never claims
      // a Learn-mode instance.
      if (
        parsed.event !== 'press'
      ) {
        return
      }

      this.config.learnedIp =
        sourceIp

      this.config.deviceId =
        parsed.deviceId

      this.config.relearn =
        false

      this.deviceId =
        parsed.deviceId

      this.saveConfig({
        ...this.config,
      })

      this.log(
        'info',
        `Paired ${parsed.deviceId} at ${sourceIp}`,
      )
    } else if (
      this.config.pairingMode === 'manual' &&
      this.config.manualIp &&
      !this.deviceId
    ) {
      // Manual mode knows the IP.
      // Any valid HotButton heartbeat/button packet from that
      // exact IP validates and refreshes the Device ID.
      if (
        sourceIp !==
        this.config.manualIp
      ) {
        return
      }

      this.deviceId =
        parsed.deviceId

      this.config.deviceId =
        parsed.deviceId

      this.saveConfig({
        ...this.config,
      })

      this.log(
        'info',
        `Learned Device ID ${parsed.deviceId} from manual IP ${sourceIp}`,
      )
    }

    const targetIp =
      this.getTargetIp()

    const expectedId =
      this.deviceId ||
      this.config.deviceId

    if (
      !targetIp ||
      !expectedId
    ) {
      return
    }

    if (
      sourceIp !== targetIp ||
      parsed.deviceId !== expectedId
    ) {
      return
    }

    this.lastSeenAt =
      Date.now()

    if (!this.online) {
      this.online = true

      this.checkFeedbacks(
        'device_online',
      )
    }

    if (
      parsed.event === 'press'
    ) {
      this.handlePressEvent()
    } else if (
      parsed.event === 'release'
    ) {
      this.handleReleaseEvent()
    } else {
      const heartbeatState =
        osc.args.length === 1 &&
        osc.args[0].type === 'i'
          ? Number(
              osc.args[0].value,
            )
          : null

      this.handleHeartbeatEvent(
        heartbeatState,
      )
    }

    this.checkFeedbacks(
      'device_online',
    )

    this.refreshStatus()
  }

  handlePressEvent() {
    const now =
      Date.now()

    if (
      this.previousPressAt > 0
    ) {
      this.lastPressIntervalMs =
        Math.max(
          0,
          now -
            this.previousPressAt,
        )
    }

    this.previousPressAt =
      now

    this.lastPressAt =
      now

    this.pressStartedAt =
      now

    this.buttonState =
      'pressed'

    this.longPressQualified =
      false

    this.longPressActive =
      false

    this.clearLongPressTimers()

    this.updateVariables(
      'press',
    )

    this.checkFeedbacks(
      'button_press_pulse',
      'button_long_press',
    )

    this.armLongPressTimer()
  }

  armLongPressTimer() {
    if (
      this.longPressTimer
    ) {
      clearTimeout(
        this.longPressTimer,
      )
    }

    this.longPressTimer =
      null

    if (
      this.buttonState !==
        'pressed' ||
      !this.pressStartedAt
    ) {
      return
    }

    const threshold =
      Math.max(
        0,
        Number(
          this.config
            .longPressThreshold ??
            DEFAULT_LONG_PRESS_MS,
        ),
      )

    const elapsed =
      Date.now() -
      this.pressStartedAt

    const remaining =
      Math.max(
        0,
        threshold -
          elapsed,
      )

    this.longPressTimer =
      setTimeout(() => {
        this.longPressTimer =
          null

        if (
          this.buttonState !==
          'pressed'
        ) {
          return
        }

        this.longPressQualified =
          true

        if (
          this.config
            .longPressWhileHeld
        ) {
          this.longPressActive =
            true

          this.updateVariables(
            'long_press',
          )

          this.checkFeedbacks(
            'button_long_press',
          )
        }
      }, remaining)
  }

  handleReleaseEvent() {
    const now =
      Date.now()

    const hadPressStart =
      this.pressStartedAt > 0

    if (hadPressStart) {
      this.lastPressDurationMs =
        Math.max(
          0,
          now -
            this.pressStartedAt,
        )
    }

    if (
      this.longPressTimer
    ) {
      clearTimeout(
        this.longPressTimer,
      )
    }

    this.longPressTimer =
      null

    const triggerOnRelease =
      this.longPressQualified &&
      !this.config
        .longPressWhileHeld

    this.buttonState =
      'released'

    this.pressStartedAt =
      0

    if (triggerOnRelease) {
      this.longPressActive =
        true

      this.updateVariables(
        'long_press',
      )

      this.checkFeedbacks(
        'button_long_press',
      )

      if (
        this.longPressPulseTimer
      ) {
        clearTimeout(
          this.longPressPulseTimer,
        )
      }

      this.longPressPulseTimer =
        setTimeout(() => {
          this.longPressPulseTimer =
            null

          this.longPressActive =
            false

          this.longPressQualified =
            false

          this.checkFeedbacks(
            'button_long_press',
          )
        }, RELEASE_LONG_PRESS_PULSE_MS)
    } else {
      this.longPressActive =
        false

      this.longPressQualified =
        false

      this.updateVariables(
        'release',
      )

      this.checkFeedbacks(
        'button_long_press',
      )
    }
  }

  handleHeartbeatEvent(buttonState) {
    // Heartbeat is primarily presence/online detection.
    // The integer state is a safety net for a lost UDP release packet.
    // Most heartbeats will naturally contain 0 because normal
    // button presses are short.

    if (
      buttonState === 0 &&
      this.buttonState ===
        'pressed'
    ) {
      if (
        this.longPressTimer
      ) {
        clearTimeout(
          this.longPressTimer,
        )
      }

      this.longPressTimer =
        null

      this.buttonState =
        'released'

      this.pressStartedAt =
        0

      this.longPressQualified =
        false

      this.longPressActive =
        false

      this.updateVariables(
        'heartbeat_sync_release',
      )

      this.checkFeedbacks(
        'button_long_press',
      )

      return
    }

    if (
      buttonState === 0 &&
      this.buttonState === ''
    ) {
      this.buttonState =
        'released'
    }

    this.updateVariables(
      'heartbeat',
    )
  }

  sendOsc(suffix, args = []) {
    const ip =
      this.getTargetIp()

    const deviceId =
      this.deviceId ||
      this.config.deviceId

    if (
      !ip ||
      !deviceId
    ) {
      this.log(
        'warn',
        `Cannot send ${suffix}: HotButton is not paired yet`,
      )

      return false
    }

    const port =
      Number(
        this.config.oscPort ||
        DEFAULT_PORT,
      )

    const path =
      `/${deviceId}${suffix}`

    this.oscSend(
      ip,
      port,
      path,
      args,
    )

    return true
  }

  clampInt(value, min, max) {
    const number =
      Number(value)

    if (
      !Number.isFinite(
        number,
      )
    ) {
      return min
    }

    return Math.max(
      min,
      Math.min(
        max,
        Math.round(
          number,
        ),
      ),
    )
  }

  isIPv4(value) {
    const parts =
      String(
        value,
      ).split('.')

    return (
      parts.length === 4 &&
      parts.every(
        (part) =>
          /^\d{1,3}$/.test(
            part,
          ) &&
          Number(part) >= 0 &&
          Number(part) <= 255,
      )
    )
  }
}

export default HotButtonInstance
export const UpgradeScripts = []