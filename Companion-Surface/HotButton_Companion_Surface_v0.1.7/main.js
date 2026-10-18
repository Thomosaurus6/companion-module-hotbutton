import { createRequire as __esbuild_createRequire } from 'module';
import { fileURLToPath as __esbuild_fileURLToPath } from 'url';
import { dirname as __esbuild_dirname } from 'path';
const require = __esbuild_createRequire(import.meta.url);
const __filename = __esbuild_fileURLToPath(import.meta.url);
const __dirname = __esbuild_dirname(__filename);

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
    try {
      socket = dgram.createSocket("udp4");
    } catch (error) {
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
    try {
      socket.bind(PORT, "127.0.0.1");
    } catch (error) {
      console.error("[HotButton Surface] UDP bind failed:", error);
      this.active = false;
      try {
        socket.close();
      } catch {
      }
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
      device = { id: msg.deviceId, name: typeof msg.deviceName === "string" ? msg.deviceName.trim().slice(0, 80) : "", lastSeen: Date.now(), instance: null, down: false, offline: false };
      this.devices.set(device.id, device);
      console.log(`[HotButton Surface] Surface detected ${new Date().toISOString()} ${device.id} name=${JSON.stringify(device.name)}`);
      this.emit("surfacesAdded", [{
        surfaceId: device.id,
        deviceHandle: device.id,
        description: device.name || `HotButton ${device.id.replace(/^hotbutton_/, "")}`,
        pluginInfo: { deviceId: device.id }
      }]);
    }
    // The stable hardware ID remains the surface identity; the friendly name is display-only.
    const newName = typeof msg.deviceName === "string" ? msg.deviceName.trim().slice(0, 80) : "";
    if (newName && newName !== device.name) {
      device.name = newName;
      // Reannounce the same stable surface ID to refresh its discovery description.
      this.emit("surfacesAdded", [{
        surfaceId: device.id,
        deviceHandle: device.id,
        description: device.name,
        pluginInfo: { deviceId: device.id }
      }]);
    }
    if (device.offline) {
      device.offline = false;
      console.log(`[HotButton Surface] Surface reconnected ${new Date().toISOString()} ${device.id}`);
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
      if (now - device.lastSeen < TIMEOUT_MS || device.offline) continue;
      // Preserve Companion's opened surface across temporary hardware power loss.
      // Releasing a held key prevents a stuck-down state while offline.
      if (device.down) device.instance?.context.keyUpById(CONTROL_ID);
      device.down = false;
      device.offline = true;
      console.log(`[HotButton Surface] Surface offline ${new Date().toISOString()} ${id} after ${now - device.lastSeen}ms (registration retained)`);
    }
  }
  async triggerScan() {
    for (const device of this.devices.values()) {
      this.emit("surfacesAdded", [{
        surfaceId: device.id,
        deviceHandle: device.id,
        description: device.name || `HotButton ${device.id.replace(/^hotbutton_/, "")}`,
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
    try {
      detection.start();
    } catch (error) {
      console.error("[HotButton Surface] Detection start failed:", error);
    }
  },
  async destroy() {
    console.log("[HotButton Surface] Plugin destroy");
    detection.stop();
  },
  checkSupportsHidDevice() {
    return null;
  },
  async openSurface(surfaceId, pluginInfo, context) {
    console.log(`[HotButton Surface] Surface opened ${new Date().toISOString()} ${surfaceId}`);
    const device = detection.devices.get(pluginInfo.deviceId);
    if (!device) throw new Error(`HotButton ${pluginInfo.deviceId} is offline`);
    const surface = {
      surfaceId,
      productName: device.name || "HotButton (OSC Bridge)",
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
var main_default = plugin;
export {
  main_default as default,
  plugin
};
//# sourceMappingURL=data:application/json;base64,ewogICJ2ZXJzaW9uIjogMywKICAic291cmNlcyI6IFsiLi4vLi4vbWFpbi5qcyJdLAogICJzb3VyY2VzQ29udGVudCI6IFsiLy8gbWFpbi5qc1xuaW1wb3J0IGRncmFtIGZyb20gXCJub2RlOmRncmFtXCI7XG5pbXBvcnQgeyBFdmVudEVtaXR0ZXIgfSBmcm9tIFwibm9kZTpldmVudHNcIjtcbnZhciBQT1JUID0gMTc3NjI7XG52YXIgVElNRU9VVF9NUyA9IDE1ZTM7XG52YXIgQ09OVFJPTF9JRCA9IFwiMC8wXCI7XG52YXIgSG90QnV0dG9uRGV0ZWN0aW9uID0gY2xhc3MgZXh0ZW5kcyBFdmVudEVtaXR0ZXIge1xuICBjb25zdHJ1Y3RvcigpIHtcbiAgICBzdXBlcigpO1xuICAgIHRoaXMuZGV2aWNlcyA9IC8qIEBfX1BVUkVfXyAqLyBuZXcgTWFwKCk7XG4gICAgdGhpcy5zb2NrZXQgPSBudWxsO1xuICAgIHRoaXMudGltZXIgPSBudWxsO1xuICAgIHRoaXMuYWN0aXZlID0gZmFsc2U7XG4gIH1cbiAgc3RhcnQoKSB7XG4gICAgaWYgKHRoaXMuYWN0aXZlKSByZXR1cm47XG4gICAgdGhpcy5hY3RpdmUgPSB0cnVlO1xuICAgIGNvbnNvbGUubG9nKFwiW0hvdEJ1dHRvbiBTdXJmYWNlXSBTdGFydGluZyBVRFAgbGlzdGVuZXIgMTI3LjAuMC4xOjE3NzYyXCIpO1xuICAgIGxldCBzb2NrZXQ7XG4gICAgdHJ5IHsgc29ja2V0ID0gZGdyYW0uY3JlYXRlU29ja2V0KFwidWRwNFwiKTsgfSBjYXRjaCAoZXJyb3IpIHtcbiAgICAgIHRoaXMuYWN0aXZlID0gZmFsc2U7XG4gICAgICBjb25zb2xlLmVycm9yKFwiW0hvdEJ1dHRvbiBTdXJmYWNlXSBDYW5ub3QgY3JlYXRlIFVEUCBzb2NrZXQ6XCIsIGVycm9yKTtcbiAgICAgIHJldHVybjtcbiAgICB9XG4gICAgdGhpcy5zb2NrZXQgPSBzb2NrZXQ7XG4gICAgc29ja2V0Lm9uKFwibWVzc2FnZVwiLCAoYnVmLCByaW5mbykgPT4gdGhpcy5yZWNlaXZlKGJ1ZiwgcmluZm8pKTtcbiAgICBzb2NrZXQub24oXCJlcnJvclwiLCAoZXJyb3IpID0+IHtcbiAgICAgIGNvbnNvbGUud2FybihcIltIb3RCdXR0b24gU3VyZmFjZV0gVURQIGxpc3RlbmVyIGVycm9yOlwiLCBlcnJvci5tZXNzYWdlKTtcbiAgICAgIHRyeSB7XG4gICAgICAgIHNvY2tldC5jbG9zZSgpO1xuICAgICAgfSBjYXRjaCB7XG4gICAgICB9XG4gICAgICBpZiAodGhpcy5zb2NrZXQgPT09IHNvY2tldCkgdGhpcy5zb2NrZXQgPSBudWxsO1xuICAgIH0pO1xuICAgIHNvY2tldC5vbihcImxpc3RlbmluZ1wiLCAoKSA9PiBjb25zb2xlLmxvZyhcIltIb3RCdXR0b24gU3VyZmFjZV0gVURQIGxpc3RlbmVyIHJlYWR5XCIpKTtcbiAgICB0cnkgeyBzb2NrZXQuYmluZChQT1JULCBcIjEyNy4wLjAuMVwiKTsgfSBjYXRjaCAoZXJyb3IpIHtcbiAgICAgIGNvbnNvbGUuZXJyb3IoXCJbSG90QnV0dG9uIFN1cmZhY2VdIFVEUCBiaW5kIGZhaWxlZDpcIiwgZXJyb3IpO1xuICAgICAgdGhpcy5hY3RpdmUgPSBmYWxzZTtcbiAgICAgIHRyeSB7IHNvY2tldC5jbG9zZSgpOyB9IGNhdGNoIHt9XG4gICAgICByZXR1cm47XG4gICAgfVxuICAgIHRoaXMudGltZXIgPSBzZXRJbnRlcnZhbCgoKSA9PiB0aGlzLmV4cGlyZSgpLCAxZTMpO1xuICB9XG4gIHN0b3AoKSB7XG4gICAgdGhpcy5hY3RpdmUgPSBmYWxzZTtcbiAgICBpZiAodGhpcy50aW1lcikgY2xlYXJJbnRlcnZhbCh0aGlzLnRpbWVyKTtcbiAgICB0aGlzLnRpbWVyID0gbnVsbDtcbiAgICBpZiAodGhpcy5zb2NrZXQpIHtcbiAgICAgIHRyeSB7XG4gICAgICAgIHRoaXMuc29ja2V0LmNsb3NlKCk7XG4gICAgICB9IGNhdGNoIHtcbiAgICAgIH1cbiAgICAgIHRoaXMuc29ja2V0ID0gbnVsbDtcbiAgICB9XG4gICAgZm9yIChjb25zdCBkZXZpY2Ugb2YgdGhpcy5kZXZpY2VzLnZhbHVlcygpKSB7XG4gICAgICBpZiAoZGV2aWNlLmluc3RhbmNlKSBkZXZpY2UuaW5zdGFuY2UuY29udGV4dC5kaXNjb25uZWN0KG5ldyBFcnJvcihcIlN1cmZhY2UgcGx1Z2luIHN0b3BwZWRcIikpO1xuICAgIH1cbiAgICB0aGlzLmRldmljZXMuY2xlYXIoKTtcbiAgfVxuICByZWNlaXZlKGJ1ZmZlciwgcmluZm8pIHtcbiAgICBpZiAocmluZm8uYWRkcmVzcyAhPT0gXCIxMjcuMC4wLjFcIiB8fCBidWZmZXIubGVuZ3RoID4gMjA0OCkgcmV0dXJuO1xuICAgIGxldCBtc2c7XG4gICAgdHJ5IHtcbiAgICAgIG1zZyA9IEpTT04ucGFyc2UoYnVmZmVyLnRvU3RyaW5nKFwidXRmOFwiKSk7XG4gICAgfSBjYXRjaCB7XG4gICAgICByZXR1cm47XG4gICAgfVxuICAgIGlmIChtc2c/LnYgIT09IDEgfHwgIS9eaG90YnV0dG9uX1tBLVphLXowLTlfLV0rJC8udGVzdChtc2cuZGV2aWNlSWQpKSByZXR1cm47XG4gICAgaWYgKCFbXCJhbGl2ZVwiLCBcInByZXNzXCIsIFwicmVsZWFzZVwiXS5pbmNsdWRlcyhtc2cuZXZlbnQpKSByZXR1cm47XG4gICAgbGV0IGRldmljZSA9IHRoaXMuZGV2aWNlcy5nZXQobXNnLmRldmljZUlkKTtcbiAgICBpZiAoIWRldmljZSkge1xuICAgICAgZGV2aWNlID0geyBpZDogbXNnLmRldmljZUlkLCBsYXN0U2VlbjogRGF0ZS5ub3coKSwgaW5zdGFuY2U6IG51bGwsIGRvd246IGZhbHNlIH07XG4gICAgICB0aGlzLmRldmljZXMuc2V0KGRldmljZS5pZCwgZGV2aWNlKTtcbiAgICAgIHRoaXMuZW1pdChcInN1cmZhY2VzQWRkZWRcIiwgW3tcbiAgICAgICAgc3VyZmFjZUlkOiBkZXZpY2UuaWQsXG4gICAgICAgIGRldmljZUhhbmRsZTogZGV2aWNlLmlkLFxuICAgICAgICBkZXNjcmlwdGlvbjogYEhvdEJ1dHRvbiAke2RldmljZS5pZC5yZXBsYWNlKC9eaG90YnV0dG9uXy8sIFwiXCIpfWAsXG4gICAgICAgIHBsdWdpbkluZm86IHsgZGV2aWNlSWQ6IGRldmljZS5pZCB9XG4gICAgICB9XSk7XG4gICAgfVxuICAgIGRldmljZS5sYXN0U2VlbiA9IERhdGUubm93KCk7XG4gICAgaWYgKG1zZy5ldmVudCA9PT0gXCJwcmVzc1wiICYmICFkZXZpY2UuZG93bikge1xuICAgICAgZGV2aWNlLmRvd24gPSB0cnVlO1xuICAgICAgZGV2aWNlLmluc3RhbmNlPy5jb250ZXh0LmtleURvd25CeUlkKENPTlRST0xfSUQpO1xuICAgIH0gZWxzZSBpZiAobXNnLmV2ZW50ID09PSBcInJlbGVhc2VcIiAmJiBkZXZpY2UuZG93bikge1xuICAgICAgZGV2aWNlLmRvd24gPSBmYWxzZTtcbiAgICAgIGRldmljZS5pbnN0YW5jZT8uY29udGV4dC5rZXlVcEJ5SWQoQ09OVFJPTF9JRCk7XG4gICAgfVxuICB9XG4gIGV4cGlyZSgpIHtcbiAgICBjb25zdCBub3cgPSBEYXRlLm5vdygpO1xuICAgIGZvciAoY29uc3QgW2lkLCBkZXZpY2VdIG9mIHRoaXMuZGV2aWNlcykge1xuICAgICAgaWYgKG5vdyAtIGRldmljZS5sYXN0U2VlbiA8IFRJTUVPVVRfTVMpIGNvbnRpbnVlO1xuICAgICAgaWYgKGRldmljZS5kb3duKSBkZXZpY2UuaW5zdGFuY2U/LmNvbnRleHQua2V5VXBCeUlkKENPTlRST0xfSUQpO1xuICAgICAgaWYgKGRldmljZS5pbnN0YW5jZSkgZGV2aWNlLmluc3RhbmNlLmNvbnRleHQuZGlzY29ubmVjdChuZXcgRXJyb3IoXCJIb3RCdXR0b24gY29ubmVjdGlvbiB0aW1lZCBvdXRcIikpO1xuICAgICAgdGhpcy5kZXZpY2VzLmRlbGV0ZShpZCk7XG4gICAgICB0aGlzLmVtaXQoXCJzdXJmYWNlc1JlbW92ZWRcIiwgW2lkXSk7XG4gICAgfVxuICB9XG4gIGFzeW5jIHRyaWdnZXJTY2FuKCkge1xuICAgIGZvciAoY29uc3QgZGV2aWNlIG9mIHRoaXMuZGV2aWNlcy52YWx1ZXMoKSkge1xuICAgICAgdGhpcy5lbWl0KFwic3VyZmFjZXNBZGRlZFwiLCBbe1xuICAgICAgICBzdXJmYWNlSWQ6IGRldmljZS5pZCxcbiAgICAgICAgZGV2aWNlSGFuZGxlOiBkZXZpY2UuaWQsXG4gICAgICAgIGRlc2NyaXB0aW9uOiBgSG90QnV0dG9uICR7ZGV2aWNlLmlkLnJlcGxhY2UoL15ob3RidXR0b25fLywgXCJcIil9YCxcbiAgICAgICAgcGx1Z2luSW5mbzogeyBkZXZpY2VJZDogZGV2aWNlLmlkIH1cbiAgICAgIH1dKTtcbiAgICB9XG4gIH1cbiAgcmVqZWN0U3VyZmFjZShfaW5mbykge1xuICB9XG59O1xudmFyIGRldGVjdGlvbiA9IG5ldyBIb3RCdXR0b25EZXRlY3Rpb24oKTtcbnZhciBwbHVnaW4gPSB7XG4gIGRldGVjdGlvbixcbiAgYXN5bmMgaW5pdCgpIHtcbiAgICBjb25zb2xlLmxvZyhcIltIb3RCdXR0b24gU3VyZmFjZV0gUGx1Z2luIGluaXRpYWxpemVkOyBzY2hlZHVsaW5nIGRldGVjdGlvblwiKTtcbiAgICB0cnkgeyBkZXRlY3Rpb24uc3RhcnQoKTsgfSBjYXRjaCAoZXJyb3IpIHsgY29uc29sZS5lcnJvcihcIltIb3RCdXR0b24gU3VyZmFjZV0gRGV0ZWN0aW9uIHN0YXJ0IGZhaWxlZDpcIiwgZXJyb3IpOyB9XG4gIH0sXG4gIGFzeW5jIGRlc3Ryb3koKSB7XG4gICAgY29uc29sZS5sb2coXCJbSG90QnV0dG9uIFN1cmZhY2VdIFBsdWdpbiBkZXN0cm95XCIpO1xuICAgIGRldGVjdGlvbi5zdG9wKCk7XG4gIH0sXG4gIGNoZWNrU3VwcG9ydHNIaWREZXZpY2UoKSB7XG4gICAgcmV0dXJuIG51bGw7XG4gIH0sXG4gIGFzeW5jIG9wZW5TdXJmYWNlKHN1cmZhY2VJZCwgcGx1Z2luSW5mbywgY29udGV4dCkge1xuICAgIGNvbnNvbGUubG9nKFwiW0hvdEJ1dHRvbiBTdXJmYWNlXSBPcGVuaW5nXCIsIHN1cmZhY2VJZCk7XG4gICAgY29uc3QgZGV2aWNlID0gZGV0ZWN0aW9uLmRldmljZXMuZ2V0KHBsdWdpbkluZm8uZGV2aWNlSWQpO1xuICAgIGlmICghZGV2aWNlKSB0aHJvdyBuZXcgRXJyb3IoYEhvdEJ1dHRvbiAke3BsdWdpbkluZm8uZGV2aWNlSWR9IGlzIG9mZmxpbmVgKTtcbiAgICBjb25zdCBzdXJmYWNlID0ge1xuICAgICAgc3VyZmFjZUlkLFxuICAgICAgcHJvZHVjdE5hbWU6IFwiSG90QnV0dG9uIChPU0MgQnJpZGdlKVwiLFxuICAgICAgYXN5bmMgaW5pdCgpIHtcbiAgICAgIH0sXG4gICAgICBhc3luYyByZWFkeSgpIHtcbiAgICAgIH0sXG4gICAgICBhc3luYyBkcmF3KF9zaWduYWwsIF9kcmF3UHJvcHMpIHtcbiAgICAgIH0sXG4gICAgICAvLyBMRUQgY29udHJvbCBzdGF5cyBpbiBPU0MgY29ubmVjdGlvbiBtb2R1bGUuXG4gICAgICBhc3luYyBzZXRCcmlnaHRuZXNzKF9wZXJjZW50KSB7XG4gICAgICB9LFxuICAgICAgYXN5bmMgYmxhbmsoKSB7XG4gICAgICB9LFxuICAgICAgYXN5bmMgY2xvc2UoKSB7XG4gICAgICAgIGlmIChkZXZpY2UuaW5zdGFuY2U/LnN1cmZhY2UgPT09IHN1cmZhY2UpIGRldmljZS5pbnN0YW5jZSA9IG51bGw7XG4gICAgICB9XG4gICAgfTtcbiAgICBkZXZpY2UuaW5zdGFuY2UgPSB7IHN1cmZhY2UsIGNvbnRleHQgfTtcbiAgICBpZiAoZGV2aWNlLmRvd24pIGNvbnRleHQua2V5RG93bkJ5SWQoQ09OVFJPTF9JRCk7XG4gICAgcmV0dXJuIHtcbiAgICAgIHN1cmZhY2UsXG4gICAgICByZWdpc3RlclByb3BzOiB7XG4gICAgICAgIGJyaWdodG5lc3M6IGZhbHNlLFxuICAgICAgICBzdXJmYWNlTGF5b3V0OiB7XG4gICAgICAgICAgc3R5bGVQcmVzZXRzOiB7IGRlZmF1bHQ6IHsgYml0bWFwOiB7IHc6IDcyLCBoOiA3MiwgZm9ybWF0OiBcInJnYlwiIH0gfSB9LFxuICAgICAgICAgIGNvbnRyb2xzOiB7IFtDT05UUk9MX0lEXTogeyByb3c6IDAsIGNvbHVtbjogMCB9IH1cbiAgICAgICAgfSxcbiAgICAgICAgcGluY29kZU1hcDogbnVsbCxcbiAgICAgICAgbG9jYXRpb246IG51bGwsXG4gICAgICAgIGNvbmZpZ0ZpZWxkczogbnVsbFxuICAgICAgfVxuICAgIH07XG4gIH1cbn07XG5leHBvcnQge1xuICBwbHVnaW5cbn07XG5leHBvcnQgZGVmYXVsdCBwbHVnaW47XG4iXSwKICAibWFwcGluZ3MiOiAiOzs7Ozs7OztBQUNBLE9BQU8sV0FBVztBQUNsQixTQUFTLG9CQUFvQjtBQUM3QixJQUFJLE9BQU87QUFDWCxJQUFJLGFBQWE7QUFDakIsSUFBSSxhQUFhO0FBQ2pCLElBQUkscUJBQXFCLGNBQWMsYUFBYTtBQUFBLEVBQ2xELGNBQWM7QUFDWixVQUFNO0FBQ04sU0FBSyxVQUEwQixvQkFBSSxJQUFJO0FBQ3ZDLFNBQUssU0FBUztBQUNkLFNBQUssUUFBUTtBQUNiLFNBQUssU0FBUztBQUFBLEVBQ2hCO0FBQUEsRUFDQSxRQUFRO0FBQ04sUUFBSSxLQUFLLE9BQVE7QUFDakIsU0FBSyxTQUFTO0FBQ2QsWUFBUSxJQUFJLDJEQUEyRDtBQUN2RSxRQUFJO0FBQ0osUUFBSTtBQUFFLGVBQVMsTUFBTSxhQUFhLE1BQU07QUFBQSxJQUFHLFNBQVMsT0FBTztBQUN6RCxXQUFLLFNBQVM7QUFDZCxjQUFRLE1BQU0saURBQWlELEtBQUs7QUFDcEU7QUFBQSxJQUNGO0FBQ0EsU0FBSyxTQUFTO0FBQ2QsV0FBTyxHQUFHLFdBQVcsQ0FBQyxLQUFLLFVBQVUsS0FBSyxRQUFRLEtBQUssS0FBSyxDQUFDO0FBQzdELFdBQU8sR0FBRyxTQUFTLENBQUMsVUFBVTtBQUM1QixjQUFRLEtBQUssMkNBQTJDLE1BQU0sT0FBTztBQUNyRSxVQUFJO0FBQ0YsZUFBTyxNQUFNO0FBQUEsTUFDZixRQUFRO0FBQUEsTUFDUjtBQUNBLFVBQUksS0FBSyxXQUFXLE9BQVEsTUFBSyxTQUFTO0FBQUEsSUFDNUMsQ0FBQztBQUNELFdBQU8sR0FB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
