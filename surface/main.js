// main.js
import dgram from "node:dgram";
import { EventEmitter } from "node:events";
var PORT = 17762;
var TIMEOUT_MS = 15e3;
var CONTROL_ID = "0/0";
var HotButtonDetection = class extends EventEmitter {
  constructor() {
    super();
    this.devices = /* @__PURE__ */ new Map();
    this.socket = null;
    this.timer = null;
    this.active = false;
  }
  start() {
    if (this.active) return;
    this.active = true;
    console.log("[HotButton Surface] Starting UDP listener 127.0.0.1:17762");
    let socket;
    try { socket = dgram.createSocket("udp4"); } catch (error) {
      this.active = false;
      console.error("[HotButton Surface] Cannot create UDP socket:", error);
      return;
    }
    this.socket = socket;
    socket.on("message", (buf, rinfo) => this.receive(buf, rinfo));
    socket.on("error", (error) => {
      console.warn("[HotButton Surface] UDP listener error:", error.message);
      try {
        socket.close();
      } catch {
      }
      if (this.socket === socket) this.socket = null;
    });
    socket.on("listening", () => console.log("[HotButton Surface] UDP listener ready"));
    try { socket.bind(PORT, "127.0.0.1"); } catch (error) {
      console.error("[HotButton Surface] UDP bind failed:", error);
      this.active = false;
      try { socket.close(); } catch {}
      return;
    }
    this.timer = setInterval(() => this.expire(), 1e3);
  }
  stop() {
    this.active = false;
    if (this.timer) clearInterval(this.timer);
    this.timer = null;
    if (this.socket) {
      try {
        this.socket.close();
      } catch {
      }
      this.socket = null;
    }
    for (const device of this.devices.values()) {
      if (device.instance) device.instance.context.disconnect(new Error("Surface plugin stopped"));
    }
    this.devices.clear();
  }
  receive(buffer, rinfo) {
    if (rinfo.address !== "127.0.0.1" || buffer.length > 2048) return;
    let msg;
    try {
      msg = JSON.parse(buffer.toString("utf8"));
    } catch {
      return;
    }
    if (msg?.v !== 1 || !/^hotbutton_[A-Za-z0-9_-]+$/.test(msg.deviceId)) return;
    if (!["alive", "press", "release"].includes(msg.event)) return;
    let device = this.devices.get(msg.deviceId);
    if (!device) {
      device = { id: msg.deviceId, lastSeen: Date.now(), instance: null, down: false };
      this.devices.set(device.id, device);
      this.emit("surfacesAdded", [{
        surfaceId: device.id,
        deviceHandle: device.id,
        description: `HotButton ${device.id.replace(/^hotbutton_/, "")}`,
        pluginInfo: { deviceId: device.id }
      }]);
    }
    device.lastSeen = Date.now();
    if (msg.event === "press" && !device.down) {
      device.down = true;
      device.instance?.context.keyDownById(CONTROL_ID);
    } else if (msg.event === "release" && device.down) {
      device.down = false;
      device.instance?.context.keyUpById(CONTROL_ID);
    }
  }
  expire() {
    const now = Date.now();
    for (const [id, device] of this.devices) {
      if (now - device.lastSeen < TIMEOUT_MS) continue;
      if (device.down) device.instance?.context.keyUpById(CONTROL_ID);
      if (device.instance) device.instance.context.disconnect(new Error("HotButton connection timed out"));
      this.devices.delete(id);
      this.emit("surfacesRemoved", [id]);
    }
  }
  async triggerScan() {
    for (const device of this.devices.values()) {
      this.emit("surfacesAdded", [{
        surfaceId: device.id,
        deviceHandle: device.id,
        description: `HotButton ${device.id.replace(/^hotbutton_/, "")}`,
        pluginInfo: { deviceId: device.id }
      }]);
    }
  }
  rejectSurface(_info) {
  }
};
var detection = new HotButtonDetection();
var plugin = {
  detection,
  async init() {
    console.log("[HotButton Surface] Plugin initialized; scheduling detection");
    try { detection.start(); } catch (error) { console.error("[HotButton Surface] Detection start failed:", error); }
  },
  async destroy() {
    console.log("[HotButton Surface] Plugin destroy");
    detection.stop();
  },
  checkSupportsHidDevice() {
    return null;
  },
  async openSurface(surfaceId, pluginInfo, context) {
    console.log("[HotButton Surface] Opening", surfaceId);
    const device = detection.devices.get(pluginInfo.deviceId);
    if (!device) throw new Error(`HotButton ${pluginInfo.deviceId} is offline`);
    const surface = {
      surfaceId,
      productName: "HotButton (OSC Bridge)",
      async init() {
      },
      async ready() {
      },
      async draw(_signal, _drawProps) {
      },
      // LED control stays in OSC connection module.
      async setBrightness(_percent) {
      },
      async blank() {
      },
      async close() {
        if (device.instance?.surface === surface) device.instance = null;
      }
    };
    device.instance = { surface, context };
    if (device.down) context.keyDownById(CONTROL_ID);
    return {
      surface,
      registerProps: {
        brightness: false,
        surfaceLayout: {
          stylePresets: { default: { bitmap: { w: 72, h: 72, format: "rgb" } } },
          controls: { [CONTROL_ID]: { row: 0, column: 0 } }
        },
        pincodeMap: null,
        location: null,
        configFields: null
      }
    };
  }
};
export {
  plugin
};
export default plugin;
