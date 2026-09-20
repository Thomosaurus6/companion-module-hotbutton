/*
 * ============================================================
 * HOTBUTTON OSC / PoE
 * Version 1.0 LED State Feedback
 * ============================================================
 *
 * Hardware:
 *   Waveshare RP2350-POE-ETH
 *   W6300 Ethernet Controller
 *   24x WS2812 RGB LED ring
 *   TXS0108E level shifter (or equivalent suitable 3.3V -> 5V logic shifter)
 *   Momentary push button
 *
 * Arduino core:
 *   Earle F. Philhower Arduino-Pico core with Waveshare
 *   RP2350-POE-ETH + W6300 support.
 *
 * ------------------------------------------------------------
 * WIRING
 * ------------------------------------------------------------
 *
 * BUTTON:
 *   GPIO0 ---- Button ---- GND
 *   INPUT_PULLUP is used:
 *   open = HIGH, pressed = LOW
 *
 * WS2812 LEVEL SHIFTER:
 *   Waveshare 3V3  ---- TXS0108E VA / VCCA
 *   Waveshare VSYS ---- TXS0108E VB / VCCB
 *   Waveshare GND  ---- TXS0108E GND
 *   GPIO1          ---- TXS0108E A1
 *   TXS0108E B1    ---- WS2812 DIN
 *
 *   OE must be HIGH. On the intended module this is either already
 *   pulled up or must be connected to VA / 3V3. Check the module.
 *
 * WS2812 POWER:
 *   Waveshare VSYS ---- WS2812 +5V
 *   Waveshare GND  ---- WS2812 GND
 *
 * ------------------------------------------------------------
 * NETWORK
 * ------------------------------------------------------------
 *
 * Default mode:
 *   DHCP
 *
 * If a valid static configuration is stored in flash:
 *   Static IP / subnet / gateway is used.
 *
 * OSC UDP port:
 *   13122
 *
 * This build broadcasts outgoing OSC while unpaired and switches to persistent unicast after Companion pairing. Test host from DEBUG6 was
 * the current subnet, so the Companion computer IP is not needed.
 *
 * Health check:
 *   - Physical Ethernet link loss -> immediate network error
 *   - If gateway != 0.0.0.0:
 *       ping gateway every 5 seconds
 *       after 3 consecutive failures -> network error/reconnect
 *   - If gateway == 0.0.0.0:
 *       ping check disabled, physical link is still monitored
 *
 * DHCP reconnect:
 *   - initial attempt immediately
 *   - first 3 retries every 10 seconds
 *   - afterwards every 120 seconds
 *
 * ------------------------------------------------------------
 * BUTTON OSC
 * ------------------------------------------------------------
 *
 * Press:
 *   /hotbutton_1_press
 *
 * Release:
 *   /hotbutton_1_release
 *
 * ------------------------------------------------------------
 * LED OSC CONTROL
 * ------------------------------------------------------------
 *
 * Colors:
 *   /hotbutton_1/led/red
 *   /hotbutton_1/led/green
 *   /hotbutton_1/led/blue
 *   /hotbutton_1/led/yellow
 *   /hotbutton_1/led/orange
 *   /hotbutton_1/led/purple
 *   /hotbutton_1/led/white
 *
 * Custom RGB:
 *   /hotbutton_1/led/custom      <OSC int R, int G, int B>
 *                                each value is clamped to 0..255
 *
 * Brightness:
 *   /hotbutton_1/led/brightness      <OSC int 0..100>
 *   /hotbutton_1/led/brightness/add  <OSC int amount>
 *   /hotbutton_1/led/brightness/sub  <OSC int amount>
 *
 * Flash:
 *   /hotbutton_1/led/flash       <OSC int 1..10>
 *
 * Solid:
 *   /hotbutton_1/led/solid
 *
 * Power:
 *   /hotbutton_1/led/on
 *   /hotbutton_1/led/off
 *   /hotbutton_1/led/toggle
 *
 *   OFF only disables physical LED output. Color, brightness and
 *   flash/solid mode remain stored. ON restores the stored state.
 *   TOGGLE switches between the current On and Off power state.
 *
 * Reset:
 *   /hotbutton_1/led/reset
 *
 * LED state query / feedback transport:
 *   /hotbutton_1/led/state/get
 *
 * The device replies with a complete snapshot:
 *   /hotbutton_1/led/state       <OSC int 0|1>
 *   /hotbutton_1/led/color       <OSC int R, int G, int B>
 *   /hotbutton_1/led/brightness/state <OSC int 0..100>
 *   /hotbutton_1/led/flash/state <OSC int 0..10>
 *
 * flash/state: 0 = Solid/Off flash mode, 1..10 = selected flash program.
 * A fresh snapshot is also sent after every LED control command.
 *
 *   Resets the user LED state to:
 *     Color      = White
 *     Brightness = 0
 *     Mode       = Solid
 *     Power      = Off
 *
 * MASTER_BRIGHTNESS is an additional hard cap:
 *   MASTER = 50, brightness = 100 -> 50% physical maximum
 *   MASTER = 50, brightness =  50 -> 25% physical maximum
 *
 * ------------------------------------------------------------
 * NETWORK OSC CONTROL
 * ------------------------------------------------------------
 *
 * Set and permanently store static network configuration:
 *
 *   /hotbutton_1/network/static
 *
 *   OSC arguments: 3 strings
 *     "10.1.10.50"
 *     "255.255.255.0"
 *     "10.1.10.1"
 *
 * Gateway may be "0.0.0.0". In that case gateway ping monitoring
 * is disabled.
 *
 * Return permanently to DHCP:
 *
 *   /hotbutton_1/network/dhcp
 *
 * Both commands immediately restart the network interface.
 *
 * ------------------------------------------------------------
 * HEARTBEAT
 * ------------------------------------------------------------
 *
 * While the network is usable, the device sends:
 *
 *   /hotbutton_1/heartbeat      <OSC int buttonState>
 *
 * buttonState:
 *   0 = physical button released
 *   1 = physical button pressed
 *
 * every HEARTBEAT_INTERVAL_MS (default 5 seconds). An immediate
 * heartbeat is also sent when the network becomes usable.
 *
 * Companion can use this for device discovery / Device ID learning,
 * an online timeout (recommended default: 20 seconds), and as a
 * safety synchronization mechanism if a UDP release event is lost.
 *
 * No heartbeat is sent while Ethernet/network is unavailable.
 *
 * ------------------------------------------------------------
 * HARDWARE NETWORK RESET
 * ------------------------------------------------------------
 *
 * Hold the physical button BEFORE applying power/PoE and keep it
 * pressed for NETWORK_RESET_HOLD_MS (default 5 seconds).
 *
 * Result:
 *   - saved static configuration is cleared/deactivated
 *   - device returns to DHCP mode
 *
 * Intentionally there is NO special LED feedback while holding
 * the button for this reset.
 *
 * ------------------------------------------------------------
 * STATUS LED RING
 * ------------------------------------------------------------
 *
 * No usable network:
 *   RED 2.0 s
 *   OFF 1.0 s
 *   repeat
 *
 * Network acquired/reacquired:
 *   GREEN 0.5 s
 *   OFF   1.0 s
 *   for 30 seconds total
 *
 * Afterwards:
 *   LEDs OFF
 *
 * Entering either network status sets brightness = 100.
 * MASTER_BRIGHTNESS is NEVER overwritten.
 *
 * ============================================================
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

extern "C" {
#include "src/DEV_Config.h"
#include "src/ethchip_conf.h"
#include "src/ethchip_spi.h"
#include "src/socket.h"
#include "src/dhcp.h"
}


// ============================================================
// USER CONFIGURATION
// ============================================================

// Serial debug output. Set to true only when diagnostics are needed.
static const bool debug_serial = false;

#define DEBUG_PRINT(...)   do { if (debug_serial) Serial.print(__VA_ARGS__); } while (0)
#define DEBUG_PRINTLN(...) do { if (debug_serial) Serial.println(__VA_ARGS__); } while (0)

// OSC device identifier.
// Change only this for the second unit, e.g. "hotbutton_2".
static const char DEVICE_NAME[] = "hotbutton_1";

// Network hostname. Hyphen instead of underscore for hostname compatibility.
static const char HOSTNAME[] = "hotbutton-1";

// User I/O
constexpr uint8_t BUTTON_PIN = 0;
constexpr uint8_t LED_PIN    = 1;

// Mirror the complete HotButton LED output to the onboard WS2812 on GPIO25.
// Set to false for the finished unit if only the external LED should be used.
constexpr bool USE_ONBOARD_LED = true;
constexpr uint8_t ONBOARD_LED_PIN = 25;
constexpr uint16_t LED_COUNT = 24;

// OSC
constexpr uint16_t OSC_PORT = 13122;

// Button
constexpr uint32_t DEBOUNCE_MS = 100;

// Brightness hard limit.
// This is NEVER changed at runtime by OSC or status logic.
constexpr uint8_t MASTER_BRIGHTNESS = 50;

// Current operational brightness starts at 100.
// OSC may change it to 0..100.
// Network status deliberately sets it back to 100.
uint8_t brightness = 100;


// -------------------- COLORS --------------------

struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

constexpr RGB COLOR_RED    = {255,   0,   0};
constexpr RGB COLOR_GREEN  = {  0, 255,   0};
constexpr RGB COLOR_BLUE   = {  0,   0, 255};
constexpr RGB COLOR_YELLOW = {255, 255,   0};
constexpr RGB COLOR_ORANGE = {255, 100,   0};
constexpr RGB COLOR_PURPLE = {180,   0, 255};
constexpr RGB COLOR_WHITE  = {255, 255, 255};


// ---------------- FLASH PROGRAMS ----------------
// Program 1 = slowest, program 10 = fastest.
// onTime/offTime are milliseconds.

struct FlashProgram {
  uint16_t onTime;
  uint16_t offTime;
};

constexpr FlashProgram FLASH_PROGRAMS[10] = {
  {650, 650}, // 1 = former program 5
  {500, 500}, // 2 = former program 6
  {400, 400}, // 3 = former program 7
  {300, 300}, // 4 = former program 8
  {200, 200}, // 5 = former program 10
  {150, 150}, // 6
  {120, 120}, // 7
  {100, 100}, // 8
  { 75,  75}, // 9
  { 50,  50}  // 10
};


// ---------------- NETWORK / DHCP ----------------

constexpr uint32_t DHCP_FAST_RETRY_MS = 10000;
constexpr uint8_t  DHCP_FAST_RETRIES  = 3;
constexpr uint32_t DHCP_SLOW_RETRY_MS = 120000;

constexpr uint32_t NETWORK_CHECK_INTERVAL_MS = 5000;
constexpr uint8_t  NETWORK_FAIL_LIMIT        = 3;

// ping() is synchronous. Keep timeout reasonably short so the button
// does not become unresponsive for seconds when the gateway is down.
constexpr uint32_t NETWORK_PING_TIMEOUT_MS = 500;
constexpr uint8_t  NETWORK_PING_TTL        = 64;

constexpr uint32_t NETWORK_RESET_HOLD_MS   = 5000;
constexpr uint32_t NETWORK_RESET_WINDOW_MS = 30UL * 1000UL;

// Device heartbeat. Companion default online timeout is intended to be
// 20 seconds, i.e. four missed 5-second heartbeats before marking offline.
constexpr uint32_t HEARTBEAT_INTERVAL_MS = 5000;


// ---------------- STATUS ----------------

constexpr uint32_t ERROR_RED_ON_MS  = 2000;
constexpr uint32_t ERROR_RED_OFF_MS = 1000;

constexpr uint32_t READY_GREEN_ON_MS  = 500;
constexpr uint32_t READY_GREEN_OFF_MS = 1000;
constexpr uint32_t READY_DURATION_MS  = 5000;


// ---------------- OSC BUFFER ----------------

constexpr size_t OSC_RX_BUFFER_SIZE = 384;


// ============================================================
// WAVESHARE RP2350-POE-ETH / W6300
// ============================================================
//
// Current Arduino-Pico board definition provides:
//   PIN_W6300_MISO = 16
//   PIN_W6300_CS   = 17
//   PIN_W6300_SCK  = 18
//   PIN_W6300_MOSI = 19
//   PIN_W6300_RST  = 20
//   PIN_W6300_INT  = 21
//
// Compile-time guard: selecting the proper board/core is required.

#ifndef PIN_W6300_CS
#error "PIN_W6300_* definitions missing. Select Waveshare RP2350-POE-ETH in a current Arduino-Pico core."
#endif

// WIZnet socket allocation.
// Socket 0 is reserved for DHCP; socket 1 is the OSC UDP listener.
constexpr uint8_t DHCP_SOCKET = 0;
constexpr uint8_t OSC_SOCKET  = 1;

uint8_t dhcpBuffer[2048] = {0};
eth_NetInfo netInfo = {};

bool dhcpActive = false;
bool dhcpLeaseReady = false;
uint32_t lastDhcpTickMs = 0;

Adafruit_NeoPixel pixels(
  LED_COUNT,
  LED_PIN,
  NEO_GRB + NEO_KHZ800
);

Adafruit_NeoPixel onboardPixel(
  1,
  ONBOARD_LED_PIN,
  NEO_GRB + NEO_KHZ800
);


// ============================================================
// PERSISTENT NETWORK CONFIGURATION
// ============================================================

constexpr uint32_t CONFIG_MAGIC   = 0x4842544EUL; // "HBTN"
constexpr uint16_t CONFIG_VERSION = 1;
constexpr size_t EEPROM_SIZE      = 256;
constexpr int EEPROM_ADDRESS      = 0;

struct StoredNetworkConfig {
  uint32_t magic;
  uint16_t version;
  uint8_t  staticEnabled;
  uint8_t  reserved;

  uint8_t ip[4];
  uint8_t subnet[4];
  uint8_t gateway[4];

  uint32_t checksum;
};

StoredNetworkConfig storedConfig;

// Companion pairing is stored separately from the network configuration so
// changing DHCP/static settings cannot accidentally erase the paired host.
constexpr uint32_t PAIRING_MAGIC   = 0x48425052UL; // "HBPR"
constexpr uint16_t PAIRING_VERSION = 1;
constexpr int EEPROM_PAIRING_ADDRESS = 64;

struct StoredPairingConfig {
  uint32_t magic;
  uint16_t version;
  uint8_t  paired;
  uint8_t  reserved;
  uint8_t  companionIP[4];
  uint32_t checksum;
};

StoredPairingConfig pairingConfig;


// ============================================================
// RUNTIME STATE
// ============================================================

enum class DisplayMode : uint8_t {
  NETWORK_ERROR,
  NETWORK_READY,
  USER
};

DisplayMode displayMode = DisplayMode::NETWORK_ERROR;

// Network
bool ethernetStarted = false;
bool networkOnline   = false;
bool udpStarted      = false;

uint8_t reconnectRetryCount = 0;
uint8_t gatewayFailCount    = 0;

uint32_t lastNetworkAttemptMs = 0;
uint32_t lastNetworkCheckMs   = 0;
uint32_t lastHeartbeatMs      = 0;

IPAddress broadcastIP(0, 0, 0, 0);
IPAddress activeGateway(0, 0, 0, 0);
IPAddress cachedLocalIP(0, 0, 0, 0);
IPAddress cachedSubnet(0, 0, 0, 0);
IPAddress companionIP(0, 0, 0, 0);
bool companionPaired = false;

// Status animation
bool statusPhaseOn = true;
uint32_t statusPhaseChangedMs = 0;
uint32_t readyStartedMs = 0;

// Button
bool rawButtonState    = HIGH;
bool stableButtonState = HIGH;
uint32_t lastButtonEdgeMs = 0;
bool suppressButtonUntilRelease = false;

// Hardware DHCP recovery is intentionally available only during the
// first five minutes after boot. The hold detection is non-blocking so
// OSC, heartbeat and normal button processing continue while it runs.
bool networkResetHoldActive = false;
bool networkResetTriggered = false;
uint32_t networkResetHoldStartedMs = 0;

// User LED
RGB currentColor = COLOR_WHITE;
bool ledPower = false;
bool flashEnabled = false;
uint8_t flashProgram = 1;
bool flashPhaseOn = true;
uint32_t flashPhaseChangedMs = 0;


// ============================================================
// UTILITY
// ============================================================

bool isZeroIP(const IPAddress &ip) {
  return ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0;
}

void copyIPToBytes(const IPAddress &ip, uint8_t out[4]) {
  for (uint8_t i = 0; i < 4; ++i) {
    out[i] = ip[i];
  }
}

IPAddress bytesToIP(const uint8_t in[4]) {
  return IPAddress(in[0], in[1], in[2], in[3]);
}

bool sameIP(const IPAddress &a, const IPAddress &b) {
  return a[0] == b[0] &&
         a[1] == b[1] &&
         a[2] == b[2] &&
         a[3] == b[3];
}


// ============================================================
// CHECKSUM / FLASH CONFIG
// ============================================================

uint32_t fnv1a32(const uint8_t *data, size_t len) {
  uint32_t hash = 2166136261UL;

  for (size_t i = 0; i < len; ++i) {
    hash ^= data[i];
    hash *= 16777619UL;
  }

  return hash;
}

uint32_t calculateConfigChecksum(const StoredNetworkConfig &cfg) {
  return fnv1a32(
    reinterpret_cast<const uint8_t *>(&cfg),
    offsetof(StoredNetworkConfig, checksum)
  );
}

void setDefaultNetworkConfig() {
  memset(&storedConfig, 0, sizeof(storedConfig));

  storedConfig.magic         = CONFIG_MAGIC;
  storedConfig.version       = CONFIG_VERSION;
  storedConfig.staticEnabled = 0;
  storedConfig.checksum      = calculateConfigChecksum(storedConfig);
}

bool loadNetworkConfig() {
  EEPROM.get(EEPROM_ADDRESS, storedConfig);

  if (storedConfig.magic != CONFIG_MAGIC) {
    setDefaultNetworkConfig();
    return false;
  }

  if (storedConfig.version != CONFIG_VERSION) {
    setDefaultNetworkConfig();
    return false;
  }

  if (storedConfig.checksum != calculateConfigChecksum(storedConfig)) {
    setDefaultNetworkConfig();
    return false;
  }

  return true;
}

bool saveNetworkConfig() {
  storedConfig.magic    = CONFIG_MAGIC;
  storedConfig.version  = CONFIG_VERSION;
  storedConfig.checksum = calculateConfigChecksum(storedConfig);

  EEPROM.put(EEPROM_ADDRESS, storedConfig);
  return EEPROM.commit();
}

uint32_t calculatePairingChecksum(const StoredPairingConfig &cfg) {
  return fnv1a32(
    reinterpret_cast<const uint8_t *>(&cfg),
    offsetof(StoredPairingConfig, checksum)
  );
}

void setDefaultPairingConfig() {
  memset(&pairingConfig, 0, sizeof(pairingConfig));
  pairingConfig.magic = PAIRING_MAGIC;
  pairingConfig.version = PAIRING_VERSION;
  pairingConfig.checksum = calculatePairingChecksum(pairingConfig);
  companionPaired = false;
  companionIP = IPAddress(0, 0, 0, 0);
}

bool loadPairingConfig() {
  EEPROM.get(EEPROM_PAIRING_ADDRESS, pairingConfig);
  if (pairingConfig.magic != PAIRING_MAGIC ||
      pairingConfig.version != PAIRING_VERSION ||
      pairingConfig.checksum != calculatePairingChecksum(pairingConfig) ||
      pairingConfig.paired == 0) {
    setDefaultPairingConfig();
    return false;
  }

  companionIP = bytesToIP(pairingConfig.companionIP);
  if (isZeroIP(companionIP)) {
    setDefaultPairingConfig();
    return false;
  }

  companionPaired = true;
  return true;
}

bool savePairingIP(const IPAddress &ip) {
  if (isZeroIP(ip)) return false;
  memset(&pairingConfig, 0, sizeof(pairingConfig));
  pairingConfig.magic = PAIRING_MAGIC;
  pairingConfig.version = PAIRING_VERSION;
  pairingConfig.paired = 1;
  copyIPToBytes(ip, pairingConfig.companionIP);
  pairingConfig.checksum = calculatePairingChecksum(pairingConfig);
  EEPROM.put(EEPROM_PAIRING_ADDRESS, pairingConfig);
  if (!EEPROM.commit()) return false;
  companionIP = ip;
  companionPaired = true;
  return true;
}

bool clearPairingConfig() {
  setDefaultPairingConfig();
  EEPROM.put(EEPROM_PAIRING_ADDRESS, pairingConfig);
  return EEPROM.commit();
}

bool setStaticNetworkConfig(
  const IPAddress &ip,
  const IPAddress &subnet,
  const IPAddress &gateway
) {
  memset(&storedConfig, 0, sizeof(storedConfig));

  storedConfig.magic         = CONFIG_MAGIC;
  storedConfig.version       = CONFIG_VERSION;
  storedConfig.staticEnabled = 1;

  copyIPToBytes(ip,      storedConfig.ip);
  copyIPToBytes(subnet,  storedConfig.subnet);
  copyIPToBytes(gateway, storedConfig.gateway);

  return saveNetworkConfig();
}

bool setDHCPNetworkConfig() {
  setDefaultNetworkConfig();
  return saveNetworkConfig();
}


// ============================================================
// IPv4 STRING PARSING
// ============================================================

bool parseIPv4(const char *text, IPAddress &result) {
  if (!text || !*text) {
    return false;
  }

  unsigned int a, b, c, d;
  char trailing;

  int count = sscanf(
    text,
    "%u.%u.%u.%u%c",
    &a, &b, &c, &d, &trailing
  );

  if (count != 4) {
    return false;
  }

  if (a > 255 || b > 255 || c > 255 || d > 255) {
    return false;
  }

  result = IPAddress(
    static_cast<uint8_t>(a),
    static_cast<uint8_t>(b),
    static_cast<uint8_t>(c),
    static_cast<uint8_t>(d)
  );

  return true;
}


// ============================================================
// LED OUTPUT
// ============================================================
//
// We intentionally do NOT use NeoPixel.setBrightness() for runtime
// scaling. Repeated setBrightness() calls rescale the internal buffer.
// Instead, RGB values are scaled fresh every time we render.

uint8_t scaleChannel(uint8_t channel) {
  // denominator = 100 * 100
  const uint32_t numerator =
    static_cast<uint32_t>(channel) *
    static_cast<uint32_t>(brightness) *
    static_cast<uint32_t>(MASTER_BRIGHTNESS);

  return static_cast<uint8_t>((numerator + 5000UL) / 10000UL);
}

void renderColor(const RGB &color) {
  const uint8_t r = scaleChannel(color.r);
  const uint8_t g = scaleChannel(color.g);
  const uint8_t b = scaleChannel(color.b);

  const uint32_t packed = pixels.Color(r, g, b);

  for (uint16_t i = 0; i < LED_COUNT; ++i) {
    pixels.setPixelColor(i, packed);
  }

  pixels.show();

  if (USE_ONBOARD_LED) {
    // Waveshare onboard WS2812 requires red/green swapped at the API call.
    onboardPixel.setPixelColor(0, onboardPixel.Color(g, r, b));
    onboardPixel.show();
  }
}

void renderOff() {
  pixels.clear();
  pixels.show();

  if (USE_ONBOARD_LED) {
    onboardPixel.clear();
    onboardPixel.show();
  }
}

void renderUserState() {
  if (displayMode != DisplayMode::USER) {
    return;
  }

  if (!ledPower) {
    renderOff();
    return;
  }

  if (flashEnabled && !flashPhaseOn) {
    renderOff();
    return;
  }

  renderColor(currentColor);
}

uint8_t clampByte(int32_t value) {
  if (value < 0) value = 0;
  if (value > 255) value = 255;
  return static_cast<uint8_t>(value);
}

void setUserColor(const RGB &color) {
  currentColor = color;

  // Color is independent from LED power. This allows configuring a new
  // color while the LEDs are off and showing it later with /led/on.
  renderUserState();
}

void setUserCustomColor(int32_t r, int32_t g, int32_t b) {
  currentColor = {
    clampByte(r),
    clampByte(g),
    clampByte(b)
  };

  renderUserState();
}

void setUserBrightness(int32_t value) {
  if (value < 0) value = 0;
  if (value > 100) value = 100;

  brightness = static_cast<uint8_t>(value);
  renderUserState();
}

void addUserBrightness(int32_t amount) {
  // Increase/Decrease actions use a positive step size. Clamp the step
  // itself so malformed OSC cannot reverse the operation or overflow.
  if (amount < 0) amount = 0;
  if (amount > 100) amount = 100;

  const int32_t value = static_cast<int32_t>(brightness) + amount;
  setUserBrightness(value);
}

void subtractUserBrightness(int32_t amount) {
  if (amount < 0) amount = 0;
  if (amount > 100) amount = 100;

  const int32_t value = static_cast<int32_t>(brightness) - amount;
  setUserBrightness(value);
}

void setUserFlash(int32_t program) {
  if (program < 1) program = 1;
  if (program > 10) program = 10;

  flashProgram = static_cast<uint8_t>(program);
  flashEnabled = true;

  // Changing the mode does not implicitly switch LED power on.
  // If power is already on, start the selected flash program visibly.
  flashPhaseOn = true;
  flashPhaseChangedMs = millis();

  renderUserState();
}

void setUserSolid() {
  flashEnabled = false;
  flashPhaseOn = true;

  // Mode and power are deliberately independent.
  renderUserState();
}

void setUserOn() {
  ledPower = true;

  // When restoring a flash mode, restart with an ON phase so LED On is
  // immediately visible and the flash timing starts deterministically.
  if (flashEnabled) {
    flashPhaseOn = true;
    flashPhaseChangedMs = millis();
  }

  renderUserState();
}

void setUserOff() {
  // Only physical output is disabled. Color, brightness, flash/solid mode
  // and selected flash program remain untouched.
  ledPower = false;

  if (displayMode == DisplayMode::USER) {
    renderOff();
  }
}

void toggleUserPower() {
  if (ledPower) {
    setUserOff();
  } else {
    setUserOn();
  }
}
void resetUserLEDState() {
  currentColor = COLOR_WHITE;
  brightness = 0;

  flashEnabled = false;
  flashProgram = 1;
  flashPhaseOn = true;
  flashPhaseChangedMs = millis();

  ledPower = false;

  if (displayMode == DisplayMode::USER) {
    renderOff();
  }
}

void handleUserFlash() {
  if (displayMode != DisplayMode::USER) {
    return;
  }

  if (!ledPower || !flashEnabled) {
    return;
  }

  const FlashProgram &program = FLASH_PROGRAMS[flashProgram - 1];

  const uint32_t interval =
    flashPhaseOn ? program.onTime : program.offTime;

  if (millis() - flashPhaseChangedMs >= interval) {
    flashPhaseChangedMs = millis();
    flashPhaseOn = !flashPhaseOn;

    renderUserState();
  }
}


// ============================================================
// STATUS DISPLAY
// ============================================================

// Send the current user LED state to Companion after the boot status
// indication has finished, so feedbacks/variables start with the
// actual boot defaults (Off / White / current brightness / Flash Off).
void sendLEDStateSnapshot();

void enterNetworkErrorDisplay() {
  DEBUG_PRINT("STATUS [");
  DEBUG_PRINT(millis());
  DEBUG_PRINTLN(" ms]: NETWORK_ERROR -> red blink (waiting for usable IP)");

  brightness = 100;

  // Previous show state is intentionally discarded.
  ledPower = false;
  flashEnabled = false;

  displayMode = DisplayMode::NETWORK_ERROR;

  statusPhaseOn = true;
  statusPhaseChangedMs = millis();

  renderColor(COLOR_RED);
}

void enterNetworkReadyDisplay() {
  DEBUG_PRINT("STATUS [");
  DEBUG_PRINT(millis());
  DEBUG_PRINTLN(" ms]: NETWORK_READY -> green blink for 5 s");

  brightness = 100;

  displayMode = DisplayMode::NETWORK_READY;

  statusPhaseOn = true;
  statusPhaseChangedMs = millis();
  readyStartedMs = millis();

  renderColor(COLOR_GREEN);
}

void finishNetworkReadyDisplay() {
  DEBUG_PRINT("STATUS [");
  DEBUG_PRINT(millis());
  DEBUG_PRINTLN(" ms]: READY finished -> LED off / USER mode");

  // Per design: after the green status sequence, start dark.
  ledPower = false;
  flashEnabled = false;
  flashPhaseOn = false;

  displayMode = DisplayMode::USER;
  renderOff();

  // Initial boot-state synchronization for Companion. This does NOT
  // restore any previous LED state; it only reports the freshly
  // initialized defaults after the ready indication has completed.
  sendLEDStateSnapshot();
}

void handleStatusDisplay() {
  const uint32_t now = millis();

  if (displayMode == DisplayMode::NETWORK_ERROR) {
    const uint32_t interval =
      statusPhaseOn ? ERROR_RED_ON_MS : ERROR_RED_OFF_MS;

    if (now - statusPhaseChangedMs >= interval) {
      statusPhaseChangedMs = now;
      statusPhaseOn = !statusPhaseOn;

      if (statusPhaseOn) {
        renderColor(COLOR_RED);
      } else {
        renderOff();
      }
    }

    return;
  }

  if (displayMode == DisplayMode::NETWORK_READY) {
    if (now - readyStartedMs >= READY_DURATION_MS) {
      finishNetworkReadyDisplay();
      return;
    }

    const uint32_t interval =
      statusPhaseOn ? READY_GREEN_ON_MS : READY_GREEN_OFF_MS;

    if (now - statusPhaseChangedMs >= interval) {
      statusPhaseChangedMs = now;
      statusPhaseOn = !statusPhaseOn;

      if (statusPhaseOn) {
        renderColor(COLOR_GREEN);
      } else {
        renderOff();
      }
    }
  }
}


// Forward declaration: networkBecameOnline() sends an immediate heartbeat.
void sendHeartbeat();


// ============================================================
// W6300 HARDWARE / NETWORK
// ============================================================
//
// v1.0 uses Waveshare's W6300 driver from the known-good 01_MQTT
// example instead of Arduino-Pico's lwIP_w6300 QSPI implementation.

void sendHeartbeat();

void makeDeviceMac(uint8_t mac[6]) {
  // Locally administered, deterministic MAC derived from DEVICE_NAME.
  uint32_t hash = 2166136261UL;
  for (const char *p = DEVICE_NAME; *p; ++p) {
    hash ^= static_cast<uint8_t>(*p);
    hash *= 16777619UL;
  }

  mac[0] = 0x02; // locally administered, unicast
  mac[1] = 0x48; // 'H'
  mac[2] = 0x42; // 'B'
  mac[3] = static_cast<uint8_t>((hash >> 16) & 0xFF);
  mac[4] = static_cast<uint8_t>((hash >> 8) & 0xFF);
  mac[5] = static_cast<uint8_t>(hash & 0xFF);
}

void ipAddressToBytes(const IPAddress &ip, uint8_t out[4]) {
  for (uint8_t i = 0; i < 4; ++i) out[i] = ip[i];
}

IPAddress netInfoIP(const uint8_t in[4]) {
  return IPAddress(in[0], in[1], in[2], in[3]);
}

bool physicalLinkUp() {
  uint8_t link = PHY_LINK_OFF;
  if (ctlethchip(CW_GET_PHYLINK, &link) == -1) {
    return false;
  }
  return link == PHY_LINK_ON;
}

void stopUDP() {
  if (udpStarted) {
    close(OSC_SOCKET);
    udpStarted = false;
  }
}

void calculateBroadcastAddress() {
  const IPAddress ip   = netInfoIP(netInfo.ip);
  const IPAddress mask = netInfoIP(netInfo.sn);

  for (uint8_t i = 0; i < 4; ++i) {
    broadcastIP[i] =
      static_cast<uint8_t>(ip[i] | static_cast<uint8_t>(~mask[i]));
  }

  cachedLocalIP = ip;
  cachedSubnet  = mask;
}

void dhcpAssigned() {
  getIPfromDHCP(netInfo.ip);
  getGWfromDHCP(netInfo.gw);
  getSNfromDHCP(netInfo.sn);
  getDNSfromDHCP(netInfo.dns);

  netInfo.dhcp = NETINFO_DHCP;
  netInfo.ipmode = NETINFO_DHCP_V4;
  network_initialize(netInfo);
  dhcpLeaseReady = true;
}

void dhcpConflict() {
  dhcpLeaseReady = false;
}

bool startOSCUDP() {
  close(OSC_SOCKET);
  const int8_t result = socket(
    OSC_SOCKET,
    Sn_MR_UDP4,
    OSC_PORT,
    SF_IO_NONBLOCK
  );
  return result == OSC_SOCKET;
}

void beginEthernetAttempt() {
  stopUDP();

  networkOnline = false;
  gatewayFailCount = 0;
  dhcpActive = false;
  dhcpLeaseReady = false;

  close(DHCP_SOCKET);
  close(OSC_SOCKET);

  // Hardware reset and socket-memory initialization use Waveshare's
  // proven SPI0 implementation for this exact RP2350-POE-ETH board.
  ethchip_reset();
  ethchip_initialize();

  if (getCIDR() != 0x6300) {
    ethernetStarted = false;
    lastNetworkAttemptMs = millis();
    DEBUG_PRINT("W6300 CIDR error: 0x");
    DEBUG_PRINTLN(getCIDR(), HEX);
    return;
  }

  ethernetStarted = true;

  memset(&netInfo, 0, sizeof(netInfo));
  makeDeviceMac(netInfo.mac);

  if (storedConfig.staticEnabled) {
    memcpy(netInfo.ip, storedConfig.ip, 4);
    memcpy(netInfo.sn, storedConfig.subnet, 4);
    memcpy(netInfo.gw, storedConfig.gateway, 4);

    // DNS is unused by HotButton; mirror gateway when available.
    memcpy(netInfo.dns, storedConfig.gateway, 4);

    netInfo.dhcp = NETINFO_STATIC;
    netInfo.ipmode = NETINFO_STATIC_V4;
    network_initialize(netInfo);
    dhcpLeaseReady = true;
  } else {
    netInfo.dhcp = NETINFO_DHCP;
    netInfo.ipmode = NETINFO_DHCP_V4;

    DHCP_init(DHCP_SOCKET, dhcpBuffer);
    reg_dhcp_cbfunc(dhcpAssigned, dhcpAssigned, dhcpConflict);
    dhcpActive = true;
    lastDhcpTickMs = millis();
  }

  lastNetworkAttemptMs = millis();
}

void restartNetworkNow() {
  reconnectRetryCount = 0;
  gatewayFailCount = 0;

  enterNetworkErrorDisplay();
  beginEthernetAttempt();
}

void networkBecameOnline() {
  networkOnline = true;
  reconnectRetryCount = 0;
  gatewayFailCount = 0;

  activeGateway = netInfoIP(netInfo.gw);
  calculateBroadcastAddress();

  stopUDP();
  udpStarted = startOSCUDP();

  DEBUG_PRINT("OSC UDP socket: ");
  DEBUG_PRINTLN(udpStarted ? "OK" : "FAILED");

  lastNetworkCheckMs = millis();

  DEBUG_PRINT("HotButton IP: ");
  DEBUG_PRINTLN(netInfoIP(netInfo.ip));

  DEBUG_PRINT("OSC Broadcast: ");
  DEBUG_PRINT(broadcastIP[0]);
  DEBUG_PRINT('.');
  DEBUG_PRINT(broadcastIP[1]);
  DEBUG_PRINT('.');
  DEBUG_PRINT(broadcastIP[2]);
  DEBUG_PRINT('.');
  DEBUG_PRINT(broadcastIP[3]);
  DEBUG_PRINT(':');
  DEBUG_PRINTLN(OSC_PORT);

  enterNetworkReadyDisplay();

  lastHeartbeatMs = millis() - HEARTBEAT_INTERVAL_MS;
  sendHeartbeat();
}

void networkFailed() {
  if (!networkOnline && displayMode == DisplayMode::NETWORK_ERROR) {
    return;
  }

  networkOnline = false;
  stopUDP();
  DHCP_stop();
  dhcpActive = false;
  dhcpLeaseReady = false;

  enterNetworkErrorDisplay();

  beginEthernetAttempt();
  reconnectRetryCount = 0;
}

uint32_t currentReconnectInterval() {
  return
    (reconnectRetryCount < DHCP_FAST_RETRIES)
      ? DHCP_FAST_RETRY_MS
      : DHCP_SLOW_RETRY_MS;
}

void serviceDHCP() {
  if (!dhcpActive || dhcpLeaseReady) {
    return;
  }

  const uint32_t now = millis();

  while (now - lastDhcpTickMs >= 1000) {
    DHCP_time_handler();
    lastDhcpTickMs += 1000;
  }

  const uint8_t result = DHCP_run();

  if (result == DHCP_IP_LEASED) {
    if (!dhcpLeaseReady) {
      dhcpAssigned();
    }
  } else if (result == DHCP_FAILED) {
    DHCP_stop();
    dhcpActive = false;
    ethernetStarted = false;
    lastNetworkAttemptMs = millis();
  }
}

void handleNetworkAcquisition() {
  if (!ethernetStarted) {
    const uint32_t interval = currentReconnectInterval();

    if (millis() - lastNetworkAttemptMs >= interval) {
      ++reconnectRetryCount;
      beginEthernetAttempt();
    }
    return;
  }

  if (!physicalLinkUp()) {
    const uint32_t interval = currentReconnectInterval();

    if (millis() - lastNetworkAttemptMs >= interval) {
      ++reconnectRetryCount;
      beginEthernetAttempt();
    }
    return;
  }

  if (storedConfig.staticEnabled) {
    if (dhcpLeaseReady) {
      networkBecameOnline();
    }
    return;
  }

  serviceDHCP();

  if (dhcpLeaseReady) {
    networkBecameOnline();
    return;
  }

  const uint32_t interval = currentReconnectInterval();
  if (millis() - lastNetworkAttemptMs >= interval) {
    ++reconnectRetryCount;
    beginEthernetAttempt();
  }
}

bool pingGateway() {
  if (isZeroIP(activeGateway)) {
    return true;
  }

  eth_PING ping = {};
  ping.id = 0x4842;
  ping.seq = static_cast<uint16_t>(millis() & 0xFFFF);
  ping.destinfo.len = 4;

  for (uint8_t i = 0; i < 4; ++i) {
    ping.destinfo.ip[i] = activeGateway[i];
  }

  return ethchip_ping(&ping) == 0;
}

void handleNetworkHealth() {
  if (!networkOnline) {
    handleNetworkAcquisition();
    return;
  }

  // Sobald eine IP-Adresse erfolgreich vergeben wurde,
  // bleibt der HotButton online.
  //
  // Keine weitere DHCP-Prüfung.
  // Kein Gateway-Ping.
  // Keine Companion-Erreichbarkeitsprüfung.
  //
  // OSC und Heartbeat laufen unabhängig weiter.

  gatewayFailCount = 0;
}
// ============================================================
// OSC ENCODING
// ============================================================

size_t oscWritePaddedString(
  uint8_t *buffer,
  size_t bufferSize,
  size_t pos,
  const char *text
) {
  const size_t len = strlen(text) + 1;
  const size_t padded = (len + 3U) & ~3U;

  if (pos + padded > bufferSize) {
    return SIZE_MAX;
  }

  memset(buffer + pos, 0, padded);
  memcpy(buffer + pos, text, len - 1);

  return pos + padded;
}

bool sendOSCNoArguments(const char *path) {
  if (!networkOnline || !udpStarted) {
    return false;
  }

  uint8_t packet[128];
  size_t pos = 0;

  pos = oscWritePaddedString(
    packet,
    sizeof(packet),
    pos,
    path
  );

  if (pos == SIZE_MAX) {
    return false;
  }

  // OSC type tag string for zero arguments: ","
  pos = oscWritePaddedString(
    packet,
    sizeof(packet),
    pos,
    ","
  );

  if (pos == SIZE_MAX) {
    return false;
  }

  // Pairing-aware routing: broadcast until paired, then unicast to the stored Companion host.
  uint8_t destination[4];
  const IPAddress target = companionPaired ? companionIP : broadcastIP;
  copyIPToBytes(target, destination);

  const int32_t sent = sendto(
    OSC_SOCKET,
    packet,
    static_cast<uint16_t>(pos),
    destination,
    OSC_PORT,
    4
  );

  DEBUG_PRINT("OSC TX [");
  DEBUG_PRINT(millis());
  DEBUG_PRINT(" ms]: ");
  DEBUG_PRINT(path);
  DEBUG_PRINT(" -> ");
  DEBUG_PRINT(destination[0]);
  DEBUG_PRINT('.');
  DEBUG_PRINT(destination[1]);
  DEBUG_PRINT('.');
  DEBUG_PRINT(destination[2]);
  DEBUG_PRINT('.');
  DEBUG_PRINT(destination[3]);
  DEBUG_PRINT(':');
  DEBUG_PRINT(OSC_PORT);
  DEBUG_PRINT(" bytes=");
  DEBUG_PRINT(pos);
  DEBUG_PRINT(" sendto=");
  DEBUG_PRINT(sent);
  DEBUG_PRINTLN(sent == static_cast<int32_t>(pos) ? " OK" : " FAILED");

  return sent == static_cast<int32_t>(pos);
}

bool sendOSCOneInt(const char *path, int32_t value) {
  if (!networkOnline || !udpStarted) {
    return false;
  }

  uint8_t packet[128];
  size_t pos = 0;

  pos = oscWritePaddedString(
    packet,
    sizeof(packet),
    pos,
    path
  );

  if (pos == SIZE_MAX) {
    return false;
  }

  pos = oscWritePaddedString(
    packet,
    sizeof(packet),
    pos,
    ",i"
  );

  if (pos == SIZE_MAX || pos + 4 > sizeof(packet)) {
    return false;
  }

  const uint32_t u = static_cast<uint32_t>(value);
  packet[pos++] = static_cast<uint8_t>((u >> 24) & 0xFF);
  packet[pos++] = static_cast<uint8_t>((u >> 16) & 0xFF);
  packet[pos++] = static_cast<uint8_t>((u >> 8) & 0xFF);
  packet[pos++] = static_cast<uint8_t>(u & 0xFF);

  // Pairing-aware routing: broadcast until paired, then unicast to the stored Companion host.
  uint8_t destination[4];
  const IPAddress target = companionPaired ? companionIP : broadcastIP;
  copyIPToBytes(target, destination);

  const int32_t sent = sendto(
    OSC_SOCKET,
    packet,
    static_cast<uint16_t>(pos),
    destination,
    OSC_PORT,
    4
  );

  DEBUG_PRINT("OSC TX [");
  DEBUG_PRINT(millis());
  DEBUG_PRINT(" ms]: ");
  DEBUG_PRINT(path);
  DEBUG_PRINT(" value=");
  DEBUG_PRINT(value);
  DEBUG_PRINT(" -> ");
  DEBUG_PRINT(destination[0]);
  DEBUG_PRINT('.');
  DEBUG_PRINT(destination[1]);
  DEBUG_PRINT('.');
  DEBUG_PRINT(destination[2]);
  DEBUG_PRINT('.');
  DEBUG_PRINT(destination[3]);
  DEBUG_PRINT(':');
  DEBUG_PRINT(OSC_PORT);
  DEBUG_PRINT(" bytes=");
  DEBUG_PRINT(pos);
  DEBUG_PRINT(" sendto=");
  DEBUG_PRINT(sent);
  DEBUG_PRINTLN(sent == static_cast<int32_t>(pos) ? " OK" : " FAILED");

  return sent == static_cast<int32_t>(pos);
}

bool sendOSCThreeInts(const char *path, int32_t a, int32_t b, int32_t c) {
  if (!networkOnline || !udpStarted) return false;

  uint8_t packet[160];
  size_t pos = 0;
  pos = oscWritePaddedString(packet, sizeof(packet), pos, path);
  if (pos == SIZE_MAX) return false;
  pos = oscWritePaddedString(packet, sizeof(packet), pos, ",iii");
  if (pos == SIZE_MAX || pos + 12 > sizeof(packet)) return false;

  const int32_t values[3] = {a, b, c};
  for (uint8_t i = 0; i < 3; ++i) {
    const uint32_t u = static_cast<uint32_t>(values[i]);
    packet[pos++] = static_cast<uint8_t>((u >> 24) & 0xFF);
    packet[pos++] = static_cast<uint8_t>((u >> 16) & 0xFF);
    packet[pos++] = static_cast<uint8_t>((u >> 8) & 0xFF);
    packet[pos++] = static_cast<uint8_t>(u & 0xFF);
  }

  uint8_t destination[4];
  const IPAddress target = companionPaired ? companionIP : broadcastIP;
  copyIPToBytes(target, destination);
  const int32_t sent = sendto(OSC_SOCKET, packet, static_cast<uint16_t>(pos), destination, OSC_PORT, 4);

  DEBUG_PRINT("OSC TX ["); DEBUG_PRINT(millis()); DEBUG_PRINT(" ms]: ");
  DEBUG_PRINT(path); DEBUG_PRINT(" values="); DEBUG_PRINT(a); DEBUG_PRINT(','); DEBUG_PRINT(b); DEBUG_PRINT(','); DEBUG_PRINT(c);
  DEBUG_PRINT(" -> "); DEBUG_PRINT(destination[0]); DEBUG_PRINT('.'); DEBUG_PRINT(destination[1]); DEBUG_PRINT('.'); DEBUG_PRINT(destination[2]); DEBUG_PRINT('.'); DEBUG_PRINT(destination[3]);
  DEBUG_PRINT(':'); DEBUG_PRINT(OSC_PORT); DEBUG_PRINT(" bytes="); DEBUG_PRINT(pos); DEBUG_PRINT(" sendto="); DEBUG_PRINT(sent);
  DEBUG_PRINTLN(sent == static_cast<int32_t>(pos) ? " OK" : " FAILED");
  return sent == static_cast<int32_t>(pos);
}

void sendLEDStateSnapshot() {
  if (!networkOnline || !udpStarted) return;

  char path[112];
  snprintf(path, sizeof(path), "/%s/led/state", DEVICE_NAME);
  sendOSCOneInt(path, ledPower ? 1 : 0);

  snprintf(path, sizeof(path), "/%s/led/color", DEVICE_NAME);
  sendOSCThreeInts(path, currentColor.r, currentColor.g, currentColor.b);

  snprintf(path, sizeof(path), "/%s/led/brightness/state", DEVICE_NAME);
  sendOSCOneInt(path, brightness);

  snprintf(path, sizeof(path), "/%s/led/flash/state", DEVICE_NAME);
  sendOSCOneInt(path, flashEnabled ? flashProgram : 0);
}

void sendHeartbeat() {
  char path[96];

  snprintf(
    path,
    sizeof(path),
    "/%s/heartbeat",
    DEVICE_NAME
  );

  // The heartbeat carries the current debounced physical button state.
  // During the boot-reset suppression phase we deliberately report released
  // so a held reset button cannot create a phantom pressed state in Companion.
  const int32_t buttonState =
    (!suppressButtonUntilRelease && stableButtonState == LOW) ? 1 : 0;

  if (sendOSCOneInt(path, buttonState)) {
    lastHeartbeatMs = millis();
  }
}

void handleHeartbeat() {
  if (!networkOnline || !udpStarted) {
    return;
  }

  const uint32_t now = millis();

  if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
    sendHeartbeat();
  }
}


// ============================================================
// OSC DECODING
// ============================================================

bool oscReadString(
  const uint8_t *data,
  size_t dataLen,
  size_t &pos,
  char *out,
  size_t outSize
) {
  if (pos >= dataLen || outSize == 0) {
    return false;
  }

  const size_t start = pos;
  size_t end = start;

  while (end < dataLen && data[end] != 0) {
    ++end;
  }

  if (end >= dataLen) {
    return false;
  }

  const size_t stringLen = end - start;

  if (stringLen + 1 > outSize) {
    return false;
  }

  memcpy(out, data + start, stringLen);
  out[stringLen] = '\0';

  const size_t consumed = stringLen + 1;
  const size_t padded   = (consumed + 3U) & ~3U;

  if (start + padded > dataLen) {
    return false;
  }

  pos = start + padded;
  return true;
}

bool oscReadInt32(
  const uint8_t *data,
  size_t dataLen,
  size_t &pos,
  int32_t &value
) {
  if (pos + 4 > dataLen) {
    return false;
  }

  const uint32_t u =
    (static_cast<uint32_t>(data[pos])     << 24) |
    (static_cast<uint32_t>(data[pos + 1]) << 16) |
    (static_cast<uint32_t>(data[pos + 2]) << 8)  |
     static_cast<uint32_t>(data[pos + 3]);

  value = static_cast<int32_t>(u);
  pos += 4;

  return true;
}

bool pathEquals(const char *path, const char *suffix) {
  char expected[96];

  snprintf(
    expected,
    sizeof(expected),
    "/%s%s",
    DEVICE_NAME,
    suffix
  );

  return strcmp(path, expected) == 0;
}

void handleOSCMessage(
  const uint8_t *data,
  size_t dataLen,
  const IPAddress &sourceIP
) {
  size_t pos = 0;

  char path[128];
  char typeTags[32];

  if (!oscReadString(
        data, dataLen, pos,
        path, sizeof(path))) {
    return;
  }

  // OSC bundles are intentionally not used by this firmware.
  if (strcmp(path, "#bundle") == 0) {
    return;
  }

  if (!oscReadString(
        data, dataLen, pos,
        typeTags, sizeof(typeTags))) {
    return;
  }

  if (typeTags[0] != ',') {
    return;
  }

  // ----------------------------------------------------------
  // LED COLORS - no arguments
  // ----------------------------------------------------------

  if (strcmp(typeTags, ",") == 0) {
    if (pathEquals(path, "/led/red")) {
      setUserColor(COLOR_RED);
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/green")) {
      setUserColor(COLOR_GREEN);
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/blue")) {
      setUserColor(COLOR_BLUE);
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/yellow")) {
      setUserColor(COLOR_YELLOW);
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/orange")) {
      setUserColor(COLOR_ORANGE);
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/purple")) {
      setUserColor(COLOR_PURPLE);
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/white")) {
      setUserColor(COLOR_WHITE);
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/solid")) {
      setUserSolid();
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/on")) {
      setUserOn();
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/off")) {
      setUserOff();
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/toggle")) {
      toggleUserPower();
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/reset")) {
      resetUserLEDState();
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/state/get")) {
      sendLEDStateSnapshot();
      return;
    }

    // Pair with the Companion instance that sent this packet. The source IP
    // is authoritative, so Companion does not need to encode its own address
    // as an OSC argument. After EEPROM commit the ACK is sent by unicast.
    if (pathEquals(path, "/pair")) {
      if (savePairingIP(sourceIP)) {
        char ackPath[96];
        snprintf(ackPath, sizeof(ackPath), "/%s/pair/ack", DEVICE_NAME);
        sendOSCNoArguments(ackPath);
        sendLEDStateSnapshot();
        DEBUG_PRINT("PAIRING: stored Companion IP ");
        DEBUG_PRINTLN(companionIP);
      }
      return;
    }

    // Optional explicit unpair command for Companion. Clear persistent pairing
    // and acknowledge via broadcast because no unicast target remains stored.
    if (pathEquals(path, "/pair/clear")) {
      clearPairingConfig();
      char ackPath[96];
      snprintf(ackPath, sizeof(ackPath), "/%s/pair/clear/ack", DEVICE_NAME);
      sendOSCNoArguments(ackPath);
      DEBUG_PRINTLN("PAIRING: cleared; broadcast mode active");
      return;
    }

    // Return to DHCP and save that decision persistently.
    if (pathEquals(path, "/network/dhcp")) {
      setDHCPNetworkConfig();
      restartNetworkNow();
      return;
    }

    return;
  }

  // ----------------------------------------------------------
  // ONE INTEGER
  // ----------------------------------------------------------

  if (strcmp(typeTags, ",i") == 0) {
    int32_t value;

    if (!oscReadInt32(data, dataLen, pos, value)) {
      return;
    }

    if (pathEquals(path, "/led/brightness")) {
      setUserBrightness(value);
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/brightness/add")) {
      addUserBrightness(value);
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/brightness/sub")) {
      subtractUserBrightness(value);
      sendLEDStateSnapshot();
      return;
    }

    if (pathEquals(path, "/led/flash")) {
      setUserFlash(value);
      sendLEDStateSnapshot();
      return;
    }

    return;
  }

  // ----------------------------------------------------------
  // CUSTOM RGB: THREE INTEGERS
  // ----------------------------------------------------------

  if (strcmp(typeTags, ",iii") == 0 &&
      pathEquals(path, "/led/custom")) {

    int32_t r;
    int32_t g;
    int32_t b;

    if (!oscReadInt32(data, dataLen, pos, r) ||
        !oscReadInt32(data, dataLen, pos, g) ||
        !oscReadInt32(data, dataLen, pos, b)) {
      return;
    }

    setUserCustomColor(r, g, b);
    sendLEDStateSnapshot();
    return;
  }

  // ----------------------------------------------------------
  // STATIC NETWORK CONFIG: THREE STRINGS
  // ----------------------------------------------------------

  if (strcmp(typeTags, ",sss") == 0 &&
      pathEquals(path, "/network/static")) {

    char ipText[16];
    char subnetText[16];
    char gatewayText[16];

    if (!oscReadString(
          data, dataLen, pos,
          ipText, sizeof(ipText))) {
      return;
    }

    if (!oscReadString(
          data, dataLen, pos,
          subnetText, sizeof(subnetText))) {
      return;
    }

    if (!oscReadString(
          data, dataLen, pos,
          gatewayText, sizeof(gatewayText))) {
      return;
    }

    IPAddress ip;
    IPAddress subnet;
    IPAddress gateway;

    if (!parseIPv4(ipText, ip) ||
        !parseIPv4(subnetText, subnet) ||
        !parseIPv4(gatewayText, gateway)) {
      return;
    }

    // Static IP and subnet must not be 0.0.0.0.
    // Gateway is explicitly allowed to be 0.0.0.0.
    if (isZeroIP(ip) || isZeroIP(subnet)) {
      return;
    }

    setStaticNetworkConfig(ip, subnet, gateway);
    restartNetworkNow();
    return;
  }
}

void handleIncomingOSC() {
  if (!networkOnline || !udpStarted) {
    return;
  }

  uint8_t buffer[OSC_RX_BUFFER_SIZE];
  uint8_t sourceAddress[16] = {0};
  uint8_t sourceAddressLength = 4;
  uint16_t sourcePort = 0;

  while (true) {
    sourceAddressLength = 4;

    const int32_t bytesRead = recvfrom(
      OSC_SOCKET,
      buffer,
      sizeof(buffer),
      sourceAddress,
      &sourcePort,
      &sourceAddressLength
    );

    if (bytesRead <= 0) {
      break;
    }

    const IPAddress sourceIP(
      sourceAddress[0], sourceAddress[1], sourceAddress[2], sourceAddress[3]
    );

    handleOSCMessage(
      buffer,
      static_cast<size_t>(bytesRead),
      sourceIP
    );

    // A network configuration OSC message can restart the socket.
    if (!networkOnline || !udpStarted) {
      return;
    }
  }
}

// ============================================================
// BUTTON
// ============================================================

void sendButtonEvent(bool pressed) {
  char path[96];

  snprintf(
    path,
    sizeof(path),
    "/%s_%s",
    DEVICE_NAME,
    pressed ? "press" : "release"
  );

  // Single-shot button event; routing is selected automatically by pairing state.
  sendOSCNoArguments(path);
}

void handleButton() {
  const bool reading = digitalRead(BUTTON_PIN);

  // If the boot-reset procedure completed while the button was still
  // physically held, ignore everything until the first release.
  if (suppressButtonUntilRelease) {
    if (reading == HIGH) {
      suppressButtonUntilRelease = false;

      rawButtonState = HIGH;
      stableButtonState = HIGH;
      lastButtonEdgeMs = millis();
    }

    return;
  }

  if (reading != rawButtonState) {
    rawButtonState = reading;
    lastButtonEdgeMs = millis();
  }

  if ((millis() - lastButtonEdgeMs >= DEBOUNCE_MS) &&
      (stableButtonState != rawButtonState)) {

    stableButtonState = rawButtonState;

    if (networkOnline) {
      sendButtonEvent(stableButtonState == LOW);
    }
  }
}


// ============================================================
// BOOT NETWORK RESET
// ============================================================

void handleBootNetworkReset() {
  if (networkResetTriggered) {
    return;
  }

  const uint32_t now = millis();

  // Recovery is deliberately available only during the first 30 seconds
  // after boot. After that, even a very long press is only a normal
  // Companion button press / long press.
  if (now > NETWORK_RESET_WINDOW_MS) {
    networkResetHoldActive = false;
    return;
  }

  if (digitalRead(BUTTON_PIN) != LOW) {
    networkResetHoldActive = false;
    return;
  }

  if (!networkResetHoldActive) {
    networkResetHoldActive = true;
    networkResetHoldStartedMs = now;
    DEBUG_PRINT("NETWORK RESET: hold detected at ");
    DEBUG_PRINT(now);
    DEBUG_PRINTLN(" ms; hold 5 s for DHCP recovery");
    return;
  }

  if (now - networkResetHoldStartedMs < NETWORK_RESET_HOLD_MS) {
    return;
  }

  networkResetTriggered = true;
  networkResetHoldActive = false;

  DEBUG_PRINTLN("NETWORK RESET: 5 s hold reached within 30 s startup window");
  DEBUG_PRINTLN("NETWORK RESET: clearing static config -> DHCP");
  DEBUG_PRINTLN("NETWORK RESET: clearing Companion pairing -> broadcast mode");

  // If Companion already saw the press, release it cleanly before the
  // network restart. Heartbeat remains an additional safety net.
  if (networkOnline && udpStarted && stableButtonState == LOW) {
    sendButtonEvent(false);
  }

  setDHCPNetworkConfig();

  // The physical recovery reset must also forget the persisted Companion
  // destination. Otherwise a button paired to an unreachable/old Companion
  // IP would remain stuck in unicast mode even after network recovery.
  if (clearPairingConfig()) {
    DEBUG_PRINTLN("NETWORK RESET: Companion pairing cleared; broadcast mode active");
  } else {
    DEBUG_PRINTLN("NETWORK RESET ERROR: failed to persist cleared Companion pairing");
  }

  // Ignore the still-held physical button until it has been released,
  // otherwise the restart could create a second/phantom button event.
  suppressButtonUntilRelease = true;

  restartNetworkNow();
}



// ============================================================
// SETUP
// ============================================================

void setup() {
  if (debug_serial) {
    Serial.begin(115200);
  }
  delay(250);
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("=== HotButton v1.0 W6300 DEBUG3 ===");

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pixels.begin();
  pixels.clear();
  pixels.show();

  if (USE_ONBOARD_LED) {
    onboardPixel.begin();
    onboardPixel.clear();
    onboardPixel.show();
  }

  EEPROM.begin(EEPROM_SIZE);
  loadPairingConfig();
  DEBUG_PRINT("Companion pairing: ");
  if (companionPaired) {
    DEBUG_PRINT("PAIRED -> ");
    DEBUG_PRINTLN(companionIP);
  } else {
    DEBUG_PRINTLN("UNPAIRED -> broadcast mode");
  }
  loadNetworkConfig();

  rawButtonState = digitalRead(BUTTON_PIN);
  stableButtonState = rawButtonState;
  lastButtonEdgeMs = millis();

  // Initialize Waveshare's proven W6300 SPI transport once.
  // The driver uses SPI0 at 70 MHz on GPIO16/17/18/19.
  ethchip_spi_initialize();
  ethchip_cris_initialize();

  // Starting state: network unavailable -> red status.
  enterNetworkErrorDisplay();

  // Initial network attempt immediately.
  reconnectRetryCount = 0;
  beginEthernetAttempt();
}


// ============================================================
// LOOP
// ============================================================

void loop() {
  // Service W6300 link, DHCP lease and gateway health.
  handleNetworkHealth();

  // Hardware DHCP recovery: 10 s hold, but only during the first
  // five minutes after boot. This check is fully non-blocking.
  handleBootNetworkReset();

  // Button remains active as soon as a usable network exists,
  // including during the 5-second green ready indication.
  handleButton();

  // Receive OSC whenever the UDP listener is active.
  handleIncomingOSC();

  // Broadcast presence while the network is healthy.
  handleHeartbeat();

  // Status display has priority over user LED state.
  handleStatusDisplay();

  // User flashing runs only after status display has finished.
  handleUserFlash();

  // Keep loop cooperative.
  yield();
}
