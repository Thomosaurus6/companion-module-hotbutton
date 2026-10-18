/*
 * ============================================================
 * HOTBUTTON OSC / PoE
 * Version 1.2.0 – Stable Release (DHCP Fast Retry + USB Device Name)
 * ============================================================
 *
 * Hardware:
 *   Waveshare RP2350-POE-ETH
 *   W6300 Ethernet Controller
 *   40x WS2812 RGB LED ring
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
 * This build uses broadcast for discovery while unpaired and unicast heartbeat only when confirmed. Test host from DEBUG6 was
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
 * DHCP:
 *   - DHCP DISCOVER retry timeout: 2 seconds
 *   - IP conflict detection remains enabled
 *   - fallback address after 60 seconds without a lease
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
 *   /<device-id>/led/red
 *   /<device-id>/led/green
 *   /<device-id>/led/blue
 *   /<device-id>/led/yellow
 *   /<device-id>/led/orange
 *   /<device-id>/led/purple
 *   /<device-id>/led/white
 *
 * Custom RGB:
 *   /<device-id>/led/custom      <OSC int R, int G, int B>
 *                                each value is clamped to 0..255
 *
 * Brightness:
 *   /<device-id>/led/brightness      <OSC int 0..100>
 *   /<device-id>/led/brightness/add  <OSC int amount>
 *   /<device-id>/led/brightness/sub  <OSC int amount>
 *
 * Flash:
 *   /<device-id>/led/flash       <OSC int 1..10>
 *   /<device-id>/led/flash/custom <OSC int onMs, int offMs, int fadeMs>
 *   Custom: 10..60000 ms per phase, fade 0..min(onMs,offMs).
 *   Fade in at start of ON, fade out at start of OFF; all times include fades.
 *
 * Solid:
 *   /<device-id>/led/solid
 *
 * Power:
 *   /<device-id>/led/on
 *   /<device-id>/led/off
 *   /<device-id>/led/toggle
 *
 *   OFF only disables physical LED output. Color, brightness and
 *   flash/solid mode remain stored. ON restores the stored state.
 *   TOGGLE switches between the current On and Off power state.
 *
 * Reset:
 *   /<device-id>/led/reset
 *
 * LED state query / feedback transport:
 *   /<device-id>/led/state/get
 *
 * The device replies with a complete snapshot:
 *   /<device-id>/led/state       <OSC int 0|1>
 *   /<device-id>/led/color       <OSC int R, int G, int B>
 *   /<device-id>/led/brightness/state <OSC int 0..100>
 *   /<device-id>/led/flash/state <OSC int 0..11>
 *
 * flash/state: 0 = Solid, 1..10 = preset, 11 = custom.
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
 *   /<device-id>/network/static
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
 *   /<device-id>/network/dhcp
 *
 * Both commands immediately restart the network interface.
 *
 * ------------------------------------------------------------
 * HEARTBEAT
 * ------------------------------------------------------------
 *
 * While the network is usable, the device sends:
 *
 *   /<device-id>/heartbeat      <OSC int buttonState>
 *
 * buttonState:
 *   0 = physical button released
 *   1 = physical button pressed
 *
 * every HEARTBEAT_INTERVAL_MS (default 2 seconds) when connected. An immediate
 * heartbeat is only sent after Companion connection confirmation.
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
 * No IP: red smooth breathing.
 * DHCP/static ready: green Flash 10 for 1.5 s.
 * DHCP fallback: violet Flash 10 for 1.5 s.
 * Paired Companion: blue breathing while checking connection (60 s).
 * Unpaired/recovery: orange breathing while waiting for discovery.
 * Connection confirmed: green Flash 10 for 1.5 s.
 * Physical reset: red Flash 10 for 1.5 s.
 * All status displays are nonblocking; reset remains active.
 * MASTER_BRIGHTNESS is never overwritten.
 *
 * ============================================================
 */

#include "HotButtonTypes.h"
#include <Arduino.h>
#include <USB.h> // Arduino-Pico SDK USB device descriptors
#include <math.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <pico/rand.h>

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

// OSC device identifier, automatically generated from RP2350 OTP hardware ID.
static char DEVICE_NAME[48] = {0};

// Network hostname. Hyphen instead of underscore for hostname compatibility.
static char HOSTNAME[48] = {0};
static char DEVICE_DISPLAY_NAME[96] = {0};

// User I/O
constexpr uint8_t BUTTON_PIN = 0;
constexpr uint8_t LED_PIN    = 1;

// Mirror the complete HotButton LED output to the onboard WS2812 on GPIO25.
// Set to false for the finished unit if only the external LED should be used.
constexpr bool USE_ONBOARD_LED = false;
constexpr uint8_t ONBOARD_LED_PIN = 25;
constexpr uint16_t LED_COUNT = 40;

// OSC
constexpr uint16_t OSC_PORT = 13122;

// Button
constexpr uint32_t DEBOUNCE_MS = 100;

// Brightness hard limit.
// This is NEVER changed at runtime by OSC or status logic.
constexpr uint8_t MASTER_BRIGHTNESS = 60;

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
constexpr uint32_t DHCP_FALLBACK_TIMEOUT_MS = 60000;
const IPAddress DHCP_FALLBACK_IP(192, 168, 1, 10);
const IPAddress DHCP_FALLBACK_SUBNET(255, 255, 255, 0);
const IPAddress DHCP_FALLBACK_GATEWAY(0, 0, 0, 0);

constexpr uint32_t NETWORK_CHECK_INTERVAL_MS = 5000;
constexpr uint8_t  NETWORK_FAIL_LIMIT        = 3;

// ping() is synchronous. Keep timeout reasonably short so the button
// does not become unresponsive for seconds when the gateway is down.
constexpr uint32_t NETWORK_PING_TIMEOUT_MS = 500;
constexpr uint8_t  NETWORK_PING_TTL        = 64;

constexpr uint32_t NETWORK_RESET_HOLD_MS   = 5000;
constexpr uint32_t NETWORK_RESET_WINDOW_MS = 30UL * 1000UL;

// Device heartbeat. Companion default online timeout is intended to be
// 10 seconds, i.e. five missed 2-second heartbeats before marking offline.
constexpr uint32_t HEARTBEAT_INTERVAL_MS = 2000;


// ---------------- STATUS ----------------

constexpr uint32_t ERROR_RED_ON_MS  = 2000;
constexpr uint32_t ERROR_RED_OFF_MS = 1000;

constexpr uint32_t READY_GREEN_ON_MS  = 500;
constexpr uint32_t READY_GREEN_OFF_MS = 1000;
constexpr uint32_t READY_DURATION_MS  = 1500;
constexpr uint32_t FALLBACK_DURATION_MS = 1500;
constexpr uint32_t RESET_CONFIRM_DURATION_MS = 1500;
constexpr uint32_t RESET_CONFIRM_BLINK_MS = 50;
constexpr uint32_t CONNECTION_CHECK_INTERVAL_MS = 5000;
constexpr uint32_t CONNECTION_RECOVERY_AFTER_MS = 60000;
constexpr uint32_t CONNECTION_CONFIRM_DURATION_MS = 1500;
constexpr uint32_t CONNECTION_ACK_TIMEOUT_MS = 10000;
constexpr uint8_t STATUS_BRIGHTNESS = 10;
constexpr uint32_t BREATH_CYCLE_MS = 3000;


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
constexpr uint8_t HTTP_SOCKET = 2;
constexpr uint8_t MDNS_SOCKET = 3;
constexpr uint16_t MDNS_PORT = 5353;
const uint8_t MDNS_GROUP[4] = {224,0,0,251};
bool mdnsStarted = false;
uint32_t mdnsStartedAtMs = 0;
uint8_t mdnsAnnouncementCount = 0;
String mdnsHost;
constexpr uint16_t HTTP_PORT = 80;

uint8_t dhcpBuffer[2048] = {0};
eth_NetInfo netInfo = {};

bool dhcpActive = false;
bool dhcpLeaseReady = false;
bool dhcpFallbackActive = false;
bool dhcpFallbackTimerActive = false;
uint32_t dhcpFallbackStartedMs = 0;
uint32_t lastDhcpTickMs = 0;
uint32_t lastDhcpProgressLogMs = 0;

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
constexpr uint16_t PAIRING_VERSION = 1; // keep v1 layout for backward compatibility
constexpr int EEPROM_PAIRING_ADDRESS = 64;
constexpr int EEPROM_NAME_ADDRESS = 128;
constexpr uint32_t NAME_MAGIC = 0x48424E4DUL; // HBNM
constexpr uint16_t NAME_VERSION = 2; // one-time migration of old German names
struct StoredDeviceName {
  uint32_t magic;
  uint16_t version;
  char name[80];
  uint32_t checksum;
};
static_assert(EEPROM_NAME_ADDRESS + sizeof(StoredDeviceName) <= EEPROM_SIZE, "EEPROM layout overflow");

// International names, sourced from the user-edited Markdown lists.
static const char *const NAME_ADJECTIVES[] = {
  "Brave", "Clever", "Curious", "Cheerful", "Playful", "Gentle", "Mighty", "Swift",
  "Silent", "Sneaky", "Happy", "Lucky", "Sleepy", "Dreamy", "Dizzy", "Fuzzy",
  "Fluffy", "Tiny", "Giant", "Little", "Wild", "Calm", "Bold", "Shy",
  "Jolly", "Grumpy", "Silly", "Goofy", "Noble", "Royal", "Magic", "Mystic",
  "Cosmic", "Stellar", "Golden", "Silver", "Crystal", "Velvet", "Electric", "Neon",
  "Glowing", "Shimmering", "Sparkling", "Shiny", "Bright", "Dark", "Frosty", "Icy",
  "Stormy", "Misty", "Dusty", "Rusty", "Spicy", "Sweet", "Salty", "Bubbly",
  "Bouncy", "Wobbly", "Wiggly", "Jumpy", "Speedy", "Zippy", "Hyper", "Lazy",
  "Chill", "Funky", "Groovy", "Quirky", "Odd", "Epic", "Legendary", "Ancient",
  "Timeless", "Secret", "Hidden", "Lost", "Wandering", "Dancing", "Singing", "Whistling",
  "Laughing", "Smiling", "Grinning", "Floating", "Flying", "Fearless", "Drunken"
};
static const char *const NAME_NOUNS[] = {
  "Fox", "Wolf", "Bear", "Otter", "Badger", "Hedgehog", "Rabbit", "Squirrel",
  "Raccoon", "Beaver", "Moose", "Deer", "Reindeer", "Elk", "Bison", "Buffalo",
  "Boar", "Lynx", "Tiger", "Lion", "Leopard", "Panther", "Cheetah", "Jaguar",
  "Puma", "Elephant", "Rhino", "Hippo", "Giraffe", "Zebra", "Panda", "Koala",
  "Kangaroo", "Wombat", "Sloth", "Lemur", "Monkey", "Gorilla", "Chimpanzee", "Bat",
  "Owl", "Eagle", "Falcon", "Hawk", "Raven", "Crow", "Parrot", "Toucan",
  "Penguin", "Puffin", "Flamingo", "Peacock", "Swan", "Goose", "Duck", "Hummingbird",
  "Kingfisher", "Woodpecker", "Robin", "Sparrow", "Turtle", "Tortoise", "Frog", "Toad",
  "Gecko", "Chameleon", "Iguana", "Crocodile", "Alligator", "Snake", "Cobra", "Octopus",
  "Squid", "Jellyfish", "Seahorse", "Dolphin", "Whale", "Orca", "Shark", "Manta",
  "Stingray", "Crab", "Lobster", "Starfish", "Dragon", "Phoenix", "Unicorn", "Griffin",
  "Pegasus", "Goblin", "Elf", "Fairy", "Pixie", "Gnome", "Troll"
};

uint32_t fnv1a32(const uint8_t *data, size_t len);

void initDeviceIdentity() {
  const char *id = rp2040.getChipID(); // RP2350 SDK: OTP-based board ID
  if (!id || strlen(id) < 8) { Serial.println("FATAL: no valid RP2350 hardware ID"); while (true) delay(1000); }
  snprintf(DEVICE_NAME, sizeof(DEVICE_NAME), "hotbutton_%s", id);
  snprintf(HOSTNAME, sizeof(HOSTNAME), "hotbutton-%s", id);
  // Uppercase only hexadecimal hardware-ID characters; preserve the lowercase OSC prefix.
  for (char *c = DEVICE_NAME + strlen("hotbutton_"); *c; ++c) if (*c >= 'a' && *c <= 'f') *c -= 32;
  for (char *c = HOSTNAME; *c; ++c) if (*c >= 'A' && *c <= 'F') *c += 32;
}

void loadOrCreateDeviceName() {
  StoredDeviceName saved{};
  EEPROM.get(EEPROM_NAME_ADDRESS, saved);
  if (saved.magic == NAME_MAGIC && saved.version == NAME_VERSION &&
      memchr(saved.name, 0, sizeof(saved.name)) && saved.name[0] &&
      saved.checksum == fnv1a32(reinterpret_cast<const uint8_t *>(&saved), offsetof(StoredDeviceName, checksum))) {
    strncpy(DEVICE_DISPLAY_NAME, saved.name, sizeof(DEVICE_DISPLAY_NAME)-1);
    return;
  }
  // Random selection happens only once; store the result, not the seed.
  const uint32_t adjectiveIndex = get_rand_32() % (sizeof(NAME_ADJECTIVES) / sizeof(NAME_ADJECTIVES[0]));
  const uint32_t nounIndex = get_rand_32() % (sizeof(NAME_NOUNS) / sizeof(NAME_NOUNS[0]));
  snprintf(DEVICE_DISPLAY_NAME, sizeof(DEVICE_DISPLAY_NAME), "%s %s", NAME_ADJECTIVES[adjectiveIndex], NAME_NOUNS[nounIndex]);
  memset(&saved, 0, sizeof(saved));
  saved.magic = NAME_MAGIC; saved.version = NAME_VERSION;
  strncpy(saved.name, DEVICE_DISPLAY_NAME, sizeof(saved.name)-1);
  saved.checksum = fnv1a32(reinterpret_cast<const uint8_t *>(&saved), offsetof(StoredDeviceName, checksum));
  EEPROM.put(EEPROM_NAME_ADDRESS, saved);
  if (!EEPROM.commit()) DEBUG_PRINTLN("WARNING: device name could not be saved");
}


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
  FALLBACK_READY,
  CONNECTION_WAIT,
  UNPAIRED_WAIT,
  CONNECTION_CONFIRMED,
  USER
};

DisplayMode displayMode = DisplayMode::NETWORK_ERROR;

// Network
bool ethernetStarted = false;
bool networkOnline   = false;
bool udpStarted      = false;
bool httpStarted     = false;
bool webApplyPending = false;
uint32_t webApplyAtMs = 0;

uint8_t reconnectRetryCount = 0;
uint8_t gatewayFailCount    = 0;

uint32_t lastNetworkAttemptMs = 0;
uint32_t lastNetworkCheckMs   = 0;
uint32_t lastHeartbeatMs      = 0;
uint32_t lastNameSentMs = 0;

IPAddress broadcastIP(0, 0, 0, 0);
IPAddress activeGateway(0, 0, 0, 0);
IPAddress cachedLocalIP(0, 0, 0, 0);
IPAddress cachedSubnet(0, 0, 0, 0);
IPAddress companionIP(0, 0, 0, 0);
bool companionPaired = false;

// Confirmed connections are actively monitored and automatically re-established.
enum class ConnectionPhase : uint8_t { WAIT_NETWORK, CHECKING, RECOVERY, UNPAIRED, CONFIRMED, RECONNECTING };
ConnectionPhase connectionPhase = ConnectionPhase::WAIT_NETWORK;
uint32_t connectionCheckStartedMs = 0;
uint32_t lastConnectionCheckMs = 0;
uint32_t connectionConfirmStartedMs = 0;
uint32_t connectionSequence = 0;
int32_t pendingConnectionToken = 0;
bool connectionTokenPending = false;
bool connectionEstablishedThisBoot = false;
uint32_t lastValidConnectionAckMs = 0;

// Status animation
bool statusPhaseOn = true;
uint32_t statusPhaseChangedMs = 0;
uint32_t readyStartedMs = 0;
bool resetConfirmActive = false;
uint32_t resetConfirmStartedMs = 0;
uint32_t resetConfirmPhaseChangedMs = 0;
bool resetConfirmPhaseOn = true;

// Button
bool rawButtonState    = HIGH;
bool stableButtonState = HIGH;
uint32_t lastButtonEdgeMs = 0;
bool suppressButtonUntilRelease = false;

// Hardware DHCP recovery is intentionally available only during the
// first 30 seconds after boot. The hold detection is non-blocking so
// OSC, heartbeat and normal button processing continue while it runs.
bool networkResetHoldActive = false;
bool networkResetTriggered = false;
uint32_t networkResetHoldStartedMs = 0;

// User LED
RGB currentColor = COLOR_WHITE;
bool ledPower = false;
bool flashEnabled = false;
uint8_t flashProgram = 1; // 1..10 preset, 11 custom
FlashProgram customFlashProgram = {500, 500};
uint16_t customFadeMs = 100;
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
      pairingConfig.paired > 1) {
    setDefaultPairingConfig();
    return false;
  }

  companionIP = bytesToIP(pairingConfig.companionIP);
  companionPaired = pairingConfig.paired == 1 && !isZeroIP(companionIP);
  return true;
}

bool savePairingIP(const IPAddress &ip, bool paired = true) {
  if (isZeroIP(ip)) return false;
  memset(&pairingConfig, 0, sizeof(pairingConfig));
  pairingConfig.magic = PAIRING_MAGIC;
  pairingConfig.version = PAIRING_VERSION;
  pairingConfig.paired = paired ? 1 : 0;
  copyIPToBytes(ip, pairingConfig.companionIP);
  pairingConfig.checksum = calculatePairingChecksum(pairingConfig);
  EEPROM.put(EEPROM_PAIRING_ADDRESS, pairingConfig);
  if (!EEPROM.commit()) return false;
  companionIP = ip;
  companionPaired = paired;
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

uint8_t flashEnvelope = 255; // 0..255, used for preset 1..5 and custom

uint8_t scaleChannel(uint8_t channel) {
  // denominator = 100 * 100
  const uint32_t numerator =
    static_cast<uint32_t>(channel) *
    static_cast<uint32_t>(displayMode == DisplayMode::USER ? brightness : STATUS_BRIGHTNESS) *
    static_cast<uint32_t>(MASTER_BRIGHTNESS);

  const uint32_t scaled = (numerator + 5000UL) / 10000UL;
  return static_cast<uint8_t>((displayMode == DisplayMode::USER && flashEnabled && (flashProgram <= 5 || flashProgram == 11))
    ? ((scaled * flashEnvelope + 127UL) / 255UL) : scaled);
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

  if (flashEnabled && !flashPhaseOn && flashProgram != 11) {
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
  flashEnvelope = 255;

  renderUserState();
}

// Custom OSC command activates the custom program only when all values are valid.
// Invalid packets do not alter the currently selected LED program.
void setUserCustomFlash(int32_t onMs, int32_t offMs, int32_t fadeMs) {
  if (onMs < 10 || onMs > 60000 || offMs < 10 || offMs > 60000 ||
      fadeMs < 0 || fadeMs > onMs || fadeMs > offMs) return;
  customFlashProgram = {static_cast<uint16_t>(onMs), static_cast<uint16_t>(offMs)};
  customFadeMs = static_cast<uint16_t>(fadeMs);
  flashProgram = 11;
  flashEnabled = true;
  flashPhaseOn = true;
  flashPhaseChangedMs = millis();
  flashEnvelope = fadeMs == 0 ? 255 : 0;
  renderUserState();
}

void setUserSolid() {
  flashEnabled = false;
  flashPhaseOn = true;
  flashEnvelope = 255;

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
    flashEnvelope = flashProgram == 11 && customFadeMs ? 0 : 255;
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
  flashEnvelope = 255;
  flashPhaseOn = true;
  flashPhaseChangedMs = millis();

  ledPower = false;

  if (displayMode == DisplayMode::USER) {
    renderOff();
  }
}

void handleUserFlash() {
  if (displayMode != DisplayMode::USER || !ledPower || !flashEnabled) return;
  const FlashProgram &program = flashProgram == 11 ? customFlashProgram : FLASH_PROGRAMS[flashProgram - 1];
  const uint32_t now = millis();
  uint32_t interval = flashPhaseOn ? program.onTime : program.offTime;
  // Keep phase boundaries stable even if a loop iteration was delayed.
  uint32_t elapsed = now - flashPhaseChangedMs;
  if (elapsed >= interval) {
    flashPhaseChangedMs = now;
    flashPhaseOn = !flashPhaseOn;
    elapsed = 0;
    interval = flashPhaseOn ? program.onTime : program.offTime;
  }
  uint8_t target = 255;
  if (flashProgram == 11) {
    // ON starts with fade-in; OFF starts with fade-out, then stays dark.
    if (flashPhaseOn) {
      target = customFadeMs && elapsed < customFadeMs
        ? static_cast<uint8_t>(elapsed * 255UL / customFadeMs) : 255;
    } else {
      target = customFadeMs && elapsed < customFadeMs
        ? static_cast<uint8_t>(255UL - elapsed * 255UL / customFadeMs) : 0;
    }
  } else if (flashProgram <= 5 && flashPhaseOn) {
    const uint32_t remaining = elapsed < program.onTime ? program.onTime - elapsed : 0;
    const uint32_t fadeIn = 30;
    const uint32_t fadeOut = 60;
    const uint32_t rise = elapsed >= fadeIn ? 255 : elapsed * 255 / fadeIn;
    const uint32_t fall = remaining >= fadeOut ? 255 : remaining * 255 / fadeOut;
    target = static_cast<uint8_t>(rise < fall ? rise : fall);
  }
  if (target != flashEnvelope || elapsed == 0) {
    flashEnvelope = target;
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
  DEBUG_PRINTLN(" ms]: NETWORK_ERROR -> red breathing (waiting for usable IP)");

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
  DEBUG_PRINTLN(" ms]: NETWORK_READY -> green Flash 10 for 1.5 s");

  displayMode = DisplayMode::NETWORK_READY;

  statusPhaseOn = true;
  statusPhaseChangedMs = millis();
  readyStartedMs = millis();

  renderColor(COLOR_GREEN);
}

void enterFallbackReadyDisplay() {
  DEBUG_PRINT("STATUS [");
  DEBUG_PRINT(millis());
  DEBUG_PRINTLN(" ms]: DHCP FALLBACK -> violet Flash 10 for 1.5 s");

  displayMode = DisplayMode::FALLBACK_READY;
  statusPhaseOn = true;
  statusPhaseChangedMs = millis();
  readyStartedMs = millis();
  renderColor(COLOR_PURPLE);
}

// The OSC encoder is defined below; startup checks are dispatched from loop().
void sendDeviceName();
void finishNetworkReadyDisplay() {
  if (connectionEstablishedThisBoot && companionPaired && connectionPhase != ConnectionPhase::CONFIRMED) {
    connectionPhase = ConnectionPhase::RECONNECTING;
    lastConnectionCheckMs = millis() - CONNECTION_CHECK_INTERVAL_MS;
    connectionTokenPending = false;
    displayMode = DisplayMode::CONNECTION_WAIT;
    renderColor(COLOR_BLUE);
    return;
  }
  if (connectionEstablishedThisBoot) {
    displayMode = DisplayMode::USER;
    renderUserState();
    sendLEDStateSnapshot();
    return;
  }
  if (companionPaired) {
    connectionPhase = ConnectionPhase::CHECKING;
    connectionCheckStartedMs = millis();
    lastConnectionCheckMs = millis() - CONNECTION_CHECK_INTERVAL_MS;
    connectionTokenPending = false;
    displayMode = DisplayMode::CONNECTION_WAIT;
    renderColor(COLOR_BLUE);
  } else {
    connectionPhase = ConnectionPhase::UNPAIRED;
    displayMode = DisplayMode::UNPAIRED_WAIT;
    renderColor(COLOR_ORANGE);
  }
}

void confirmCompanionConnection() {
  if (connectionPhase == ConnectionPhase::CONFIRMED) return;
  connectionPhase = ConnectionPhase::CONFIRMED;
  lastValidConnectionAckMs = millis();
  connectionEstablishedThisBoot = true;
  connectionTokenPending = false;
  connectionConfirmStartedMs = millis();
  statusPhaseChangedMs = millis();
  statusPhaseOn = true;
  displayMode = DisplayMode::CONNECTION_CONFIRMED;
  renderColor(COLOR_GREEN);
  sendDeviceName();
}

void handleStatusDisplay() {
  const uint32_t now = millis();
  if (resetConfirmActive) {
    if (now - resetConfirmStartedMs < RESET_CONFIRM_DURATION_MS) {
      if (now - resetConfirmPhaseChangedMs >= RESET_CONFIRM_BLINK_MS) {
        resetConfirmPhaseChangedMs = now;
        resetConfirmPhaseOn = !resetConfirmPhaseOn;
        if (resetConfirmPhaseOn) renderColor(COLOR_RED); else renderOff();
      }
      return;
    }
    resetConfirmActive = false;
    statusPhaseChangedMs = now;
    statusPhaseOn = true;
    // Restore the active status overlay after reset confirmation.
    if (displayMode == DisplayMode::NETWORK_ERROR) renderColor(COLOR_RED);
    else if (displayMode == DisplayMode::FALLBACK_READY) renderColor(COLOR_PURPLE);
    else if (displayMode == DisplayMode::NETWORK_READY || displayMode == DisplayMode::CONNECTION_CONFIRMED) renderColor(COLOR_GREEN);
    else if (displayMode == DisplayMode::CONNECTION_WAIT) renderColor(COLOR_BLUE);
    else if (displayMode == DisplayMode::UNPAIRED_WAIT) renderColor(COLOR_ORANGE);
    else renderUserState();
  }

  if (displayMode == DisplayMode::NETWORK_READY || displayMode == DisplayMode::FALLBACK_READY || displayMode == DisplayMode::CONNECTION_CONFIRMED) {
    const uint32_t duration = displayMode == DisplayMode::NETWORK_READY ? READY_DURATION_MS :
      displayMode == DisplayMode::FALLBACK_READY ? FALLBACK_DURATION_MS : CONNECTION_CONFIRM_DURATION_MS;
    const uint32_t started = displayMode == DisplayMode::CONNECTION_CONFIRMED ? connectionConfirmStartedMs : readyStartedMs;
    if (now - started >= duration) {
      if (displayMode == DisplayMode::CONNECTION_CONFIRMED) {
        displayMode = DisplayMode::USER;
        renderUserState();
        sendLEDStateSnapshot();
      } else finishNetworkReadyDisplay();
      return;
    }
    const FlashProgram &fast = FLASH_PROGRAMS[9];
    if (now - statusPhaseChangedMs >= (statusPhaseOn ? fast.onTime : fast.offTime)) {
      statusPhaseChangedMs = now;
      statusPhaseOn = !statusPhaseOn;
      const RGB color = displayMode == DisplayMode::FALLBACK_READY ? COLOR_PURPLE : COLOR_GREEN;
      if (statusPhaseOn) renderColor(color); else renderOff();
    }
    return;
  }
  if (displayMode == DisplayMode::NETWORK_ERROR || displayMode == DisplayMode::CONNECTION_WAIT || displayMode == DisplayMode::UNPAIRED_WAIT) {
    // Triangle-free smooth sine breathing, approximately 3 seconds per cycle.
    const float phase = (float)(now % BREATH_CYCLE_MS) * (6.2831853f / (float)BREATH_CYCLE_MS);
    const float factor = 0.06f + 0.94f * (0.5f - 0.5f * cosf(phase));
    const RGB base = displayMode == DisplayMode::NETWORK_ERROR ? COLOR_RED :
      displayMode == DisplayMode::CONNECTION_WAIT ? COLOR_BLUE : COLOR_ORANGE;
    const RGB output = {(uint8_t)(base.r * factor), (uint8_t)(base.g * factor), (uint8_t)(base.b * factor)};
    static uint32_t lastBreathRenderMs = 0;
    if (now - lastBreathRenderMs >= 25) {
      lastBreathRenderMs = now;
      renderColor(output);
    }
  }
}

// ============================================================
// W6300 HARDWARE / NETWORK
// ============================================================
//
// v1.0 uses Waveshare's W6300 driver from the known-good 01_MQTT
// example instead of Arduino-Pico's lwIP_w6300 QSPI implementation.

void sendHeartbeat();

void makeDeviceMac(uint8_t mac[6]) {
  // Locally administered, deterministic MAC derived from hardware DEVICE_NAME.
  uint64_t hash = 14695981039346656037ULL;
  for (const char *p = DEVICE_NAME; *p; ++p) {
    hash ^= static_cast<uint8_t>(*p);
    hash *= 1099511628211ULL;
  }
  mac[0] = 0x02; // locally administered, unicast
  for (uint8_t i = 1; i < 6; ++i)
    mac[i] = static_cast<uint8_t>((hash >> (8 * (5 - i))) & 0xFF);
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
  if (udpStarted) { close(OSC_SOCKET); udpStarted = false; }
}
void stopHTTP() { close(HTTP_SOCKET); httpStarted = false; }
bool startHTTPServer() {
  close(HTTP_SOCKET);
  const int8_t r = socket(HTTP_SOCKET, Sn_MR_TCP4, HTTP_PORT, SF_IO_NONBLOCK);
  if (r != HTTP_SOCKET) return false;
  if (listen(HTTP_SOCKET) != SOCK_OK) { close(HTTP_SOCKET); return false; }
  httpStarted = true;
  return true;
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
  DEBUG_PRINT("DHCP: lease acquired at "); DEBUG_PRINT(millis()); DEBUG_PRINTLN(" ms");
  dhcpLeaseReady = true;
  dhcpFallbackActive = false;
  dhcpFallbackTimerActive = false;
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
  close(HTTP_SOCKET);
  httpStarted = false;

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
    dhcpFallbackActive = false;
    dhcpFallbackTimerActive = false;
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

    DEBUG_PRINT("DHCP: starting at "); DEBUG_PRINT(millis()); DEBUG_PRINTLN(" ms");
    DHCP_init(DHCP_SOCKET, dhcpBuffer);
    reg_dhcp_cbfunc(dhcpAssigned, dhcpAssigned, dhcpConflict);
    dhcpActive = true;
    dhcpFallbackActive = false;
    if (!dhcpFallbackTimerActive) {
      dhcpFallbackTimerActive = true;
      dhcpFallbackStartedMs = millis();
    }
    lastDhcpTickMs = millis();
    lastDhcpProgressLogMs = millis();
  }

  lastNetworkAttemptMs = millis();
}

void restartNetworkNow() {
  reconnectRetryCount = 0;
  dhcpFallbackActive = false;
  dhcpFallbackTimerActive = false;
  gatewayFailCount = 0;

  if (connectionEstablishedThisBoot && companionPaired) connectionPhase = ConnectionPhase::RECONNECTING;
  else connectionPhase = ConnectionPhase::WAIT_NETWORK;
  connectionTokenPending = false;
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
  httpStarted = startHTTPServer();
  startMDNS();

  DEBUG_PRINT("OSC UDP socket: ");
  DEBUG_PRINTLN(udpStarted ? "OK" : "FAILED");
  DEBUG_PRINT("HTTP server: ");
  DEBUG_PRINTLN(httpStarted ? "OK" : "FAILED");

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

  if (dhcpFallbackActive) enterFallbackReadyDisplay();
  else enterNetworkReadyDisplay();

  if (connectionEstablishedThisBoot && companionPaired) connectionPhase = ConnectionPhase::RECONNECTING;
  else connectionPhase = ConnectionPhase::WAIT_NETWORK;
  connectionTokenPending = false;
  lastHeartbeatMs = millis();
}

void networkFailed() {
  if (!networkOnline && displayMode == DisplayMode::NETWORK_ERROR) {
    return;
  }

  networkOnline = false;
  stopUDP();
  stopHTTP();
  stopMDNS();
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
    // Retry DHCP without resetting the Ethernet controller.
    DEBUG_PRINTLN("DHCP: discover timeout, restarting DHCP client");
    DHCP_stop();
    DHCP_init(DHCP_SOCKET, dhcpBuffer);
    reg_dhcp_cbfunc(dhcpAssigned, dhcpAssigned, dhcpConflict);
    lastDhcpTickMs = millis();
  }
  if (!dhcpLeaseReady && now - lastDhcpProgressLogMs >= 5000) {
    lastDhcpProgressLogMs = now;
    DEBUG_PRINT("DHCP: waiting for lease, elapsed ");
    DEBUG_PRINT((now - dhcpFallbackStartedMs) / 1000);
    DEBUG_PRINTLN(" s");
  }
}

void activateDhcpFallback() {
  DEBUG_PRINTLN("DHCP: 60 s without lease -> activating fallback 192.168.1.10/24");
  DHCP_stop();
  dhcpActive = false;

  ipAddressToBytes(DHCP_FALLBACK_IP, netInfo.ip);
  ipAddressToBytes(DHCP_FALLBACK_SUBNET, netInfo.sn);
  ipAddressToBytes(DHCP_FALLBACK_GATEWAY, netInfo.gw);
  memset(netInfo.dns, 0, sizeof(netInfo.dns));
  netInfo.dhcp = NETINFO_STATIC;
  netInfo.ipmode = NETINFO_STATIC_V4;
  network_initialize(netInfo);

  dhcpFallbackActive = true;
  dhcpFallbackTimerActive = false;
  dhcpLeaseReady = true;
}

void handleNetworkAcquisition() {
  // The DHCP fallback timeout is absolute. Check it before the DHCP/W6300
  // retry state so repeated DHCP_FAILED results cannot postpone fallback.
  if (!storedConfig.staticEnabled && dhcpFallbackTimerActive &&
      millis() - dhcpFallbackStartedMs >= DHCP_FALLBACK_TIMEOUT_MS) {
    activateDhcpFallback();
    networkBecameOnline();
    return;
  }

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

  // DHCP_run() handles its own retransmissions. Do not reset the W6300
  // every 10 seconds while the DHCP negotiation is still in progress.
  // The absolute 60-second fallback is handled above.
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

// mDNS: single-label .local A records, RFC 6762 multicast UDP/5353.
// Kept on socket 3, separate from OSC (1) and HTTP (2).
String makeMDNSHost() {
  String h = String(DEVICE_DISPLAY_NAME);
  h.toLowerCase();
  for (unsigned i=0;i<h.length();++i) {
    char c=h[i]; if (c==' ' || c=='_') h.setCharAt(i,'-');
    else if (!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-')) h.setCharAt(i,'-');
  }
  String id=String(DEVICE_NAME);
  String suffix=id.substring(id.length()>=4?id.length()-4:0);
  suffix.toLowerCase();
  return h+"-"+suffix;
}
void stopMDNS() { close(MDNS_SOCKET); mdnsStarted=false; }
bool startMDNS() {
  close(MDNS_SOCKET);
  mdnsHost=makeMDNSHost();
  // W6300 multicast group destination must be configured for the socket.
  uint8_t group[4]={224,0,0,251};
  uint8_t mac[6]={0x01,0x00,0x5e,0x00,0x00,0xfb};
  setSn_DIPR(MDNS_SOCKET,group);
  setSn_DHAR(MDNS_SOCKET,mac);
  setSn_DPORTR(MDNS_SOCKET,MDNS_PORT);
  int8_t r=socket(MDNS_SOCKET,Sn_MR_UDP4,MDNS_PORT,SF_MULTI_ENABLE|SF_IO_NONBLOCK);
  mdnsStarted=(r==MDNS_SOCKET);
  mdnsStartedAtMs = millis();
  mdnsAnnouncementCount = 0;
  return mdnsStarted;
}
static void dnsU16(uint8_t *b, size_t &n, uint16_t v) { b[n++]=v>>8; b[n++]=v&255; }
static void dnsU32(uint8_t *b, size_t &n, uint32_t v) { b[n++]=v>>24;b[n++]=v>>16;b[n++]=v>>8;b[n++]=v; }
// Decode DNS labels, including pointers; reject loops and malformed packets.
bool dnsName(const uint8_t *buf,int len,int &offset,String &out) {
  int pos=offset; bool jumped=false; int hops=0; out="";
  while (pos<len && ++hops<64) {
    uint8_t l=buf[pos++];
    if (l==0) {if(!jumped)offset=pos;return true;}
    if ((l&0xc0)==0xc0) {
      if(pos>=len)return false;
      int target=((l&0x3f)<<8)|buf[pos++];
      if(target>=len)return false;
      if(!jumped)offset=pos;
      pos=target;jumped=true;continue;
    }
    if ((l&0xc0)||l>63||pos+l>len)return false;
    if(out.length())out+='.';
    for(int i=0;i<l;++i){char c=(char)buf[pos++];out+=(char)tolower((unsigned char)c);}
    if(out.length()>150)return false;
  }
  return false;
}
void announceMDNS() {
  if (!networkOnline || !mdnsStarted || mdnsHost.length() > 63) return;
  uint8_t reply[256] = {0}; size_t p = 12;
  // Unsolicited authoritative response, cache-flush A record, TTL 120s.
  reply[2] = 0x84; reply[3] = 0x00;
  reply[6] = 0; reply[7] = 1;
  reply[p++] = static_cast<uint8_t>(mdnsHost.length());
  for (unsigned i=0; i<mdnsHost.length(); ++i) reply[p++] = mdnsHost[i];
  reply[p++] = 5; memcpy(reply+p, "local", 5); p += 5; reply[p++] = 0;
  dnsU16(reply,p,1); dnsU16(reply,p,0x8001); dnsU32(reply,p,120); dnsU16(reply,p,4);
  for (int i=0; i<4; ++i) reply[p++] = netInfo.ip[i];
  uint8_t dst[4] = {224,0,0,251};
  sendto(MDNS_SOCKET,reply,p,dst,MDNS_PORT,4);
}
void handleMDNSAnnouncements() {
  if (!networkOnline || !mdnsStarted || mdnsAnnouncementCount >= 2) return;
  uint32_t delayMs = mdnsAnnouncementCount == 0 ? 1000UL : 3000UL;
  if (millis() - mdnsStartedAtMs >= delayMs) {
    announceMDNS();
    ++mdnsAnnouncementCount;
  }
}
void handleMDNS() {
  if(!networkOnline || !mdnsStarted)return;
  if(getSn_SR(MDNS_SOCKET)==SOCK_CLOSED){startMDNS();return;}
  if(getSn_RX_RSR(MDNS_SOCKET)==0)return;
  uint8_t request[512],source[4]={0};uint16_t srcPort=0;
  uint8_t sourceAddressLength = sizeof(source);
  int32_t n=recvfrom(MDNS_SOCKET,request,sizeof(request),source,&srcPort,&sourceAddressLength);
  if(n<12)return;
  // Only standard queries. Ignore responses, probes and non-question packets.
  if(request[2]&0x80)return;
  uint16_t questions=((uint16_t)request[4]<<8)|request[5];
  if(questions==0||questions>16)return;
  String expected=mdnsHost+".local";
  bool match=false; int off=12;
  for(uint16_t i=0;i<questions;i++) {
    String name;
    if(!dnsName(request,n,off,name)||off+4>n)return;
    uint16_t type=((uint16_t)request[off]<<8)|request[off+1];
    uint16_t cls=((uint16_t)request[off+2]<<8)|request[off+3];off+=4;
    if(name==expected&&(type==1||type==255)&&(cls&0x7fff)==1)match=true;
  }
  if(!match)return;
  uint8_t reply[256]={0};size_t p=0;
  dnsU16(reply,p,0);dnsU16(reply,p,0x8400);dnsU16(reply,p,0);dnsU16(reply,p,1);
  dnsU16(reply,p,0);dnsU16(reply,p,0);
  p=12;
  if(mdnsHost.length()>63)return;
  reply[p++]=mdnsHost.length();for(unsigned i=0;i<mdnsHost.length();++i)reply[p++]=mdnsHost[i];
  reply[p++]=5;memcpy(reply+p,"local",5);p+=5;reply[p++]=0;
  dnsU16(reply,p,1);dnsU16(reply,p,0x8001);dnsU32(reply,p,120);dnsU16(reply,p,4);
  for(int i=0;i<4;++i)reply[p++]=netInfo.ip[i];
  uint8_t dst[4]={224,0,0,251};
  sendto(MDNS_SOCKET,reply,p,dst,MDNS_PORT,4);
}

// ============================================================
// HTTP NETWORK SETUP (Port 80)
// ============================================================
String ipText(const IPAddress &ip) { return String(ip[0])+"."+String(ip[1])+"."+String(ip[2])+"."+String(ip[3]); }
String queryValue(const String &q, const char *key) {
  String n=String(key)+"="; int p=0;
  while (p <= (int)q.length()) { int e=q.indexOf('&',p); if(e<0)e=q.length(); String x=q.substring(p,e); if(x.startsWith(n)) return x.substring(n.length()); p=e+1; }
  return "";
}
// Send the full HTTP response in bounded chunks. The W6300 send() implementation
// cannot safely be given an arbitrarily large HTML document in one call.
// Retry temporary nonblocking TCP backpressure instead of truncating HTML.
// Bound every retry so a disconnected browser cannot stall the main loop forever.
bool httpSendAll(const String &data) {
  const size_t chunkSize = 512;
  const uint32_t stallTimeoutMs = 3000;
  size_t offset = 0;
  uint32_t lastProgressMs = millis();
  while (offset < data.length()) {
    size_t chunk = data.length() - offset;
    if (chunk > chunkSize) chunk = chunkSize;
    const int32_t sent = send(HTTP_SOCKET,
      (uint8_t *)(data.c_str() + offset), (uint16_t)chunk);
    if (sent > 0) {
      offset += (size_t)sent;
      lastProgressMs = millis();
      continue;
    }
    const uint8_t state = getSn_SR(HTTP_SOCKET);
    if (state != SOCK_ESTABLISHED && state != SOCK_CLOSE_WAIT) {
      DEBUG_PRINT("HTTP TX: socket closed, offset="); DEBUG_PRINTLN(offset);
      return false;
    }
    if ((uint32_t)(millis() - lastProgressMs) >= stallTimeoutMs) {
      DEBUG_PRINT("HTTP TX: timeout, offset="); DEBUG_PRINT(offset);
      DEBUG_PRINT(" total="); DEBUG_PRINTLN(data.length());
      return false;
    }
    delay(2);
  }
  return true;
}
void httpReply(const String &body, const char *status="200 OK") {
  String h="HTTP/1.1 "; h+=status; h+="\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\nCache-Control: no-store\r\nContent-Length: "; h+=String(body.length()); h+="\r\n\r\n";
  if (!httpSendAll(h)) return;
  httpSendAll(body);
}
String setupPage(const String &msg="") {
  IPAddress curIP=netInfoIP(netInfo.ip), curSN=netInfoIP(netInfo.sn), curGW=netInfoIP(netInfo.gw);
  IPAddress cfgIP=storedConfig.staticEnabled?bytesToIP(storedConfig.ip):curIP;
  IPAddress cfgSN=storedConfig.staticEnabled?bytesToIP(storedConfig.subnet):curSN;
  IPAddress cfgGW=storedConfig.staticEnabled?bytesToIP(storedConfig.gateway):curGW;
  String x; x.reserve(8000);
  x += R"HBHTML(<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>HotButton Network Setup</title><style>
:root{color-scheme:dark;--bg:#0a0e16;--panel:#151c29;--line:#2b3749;--text:#edf2fb;--muted:#91a0b7;--accent:#77a9ff}*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at 50% -10%,#1b3153 0,transparent 55%),var(--bg);color:var(--text);font:15px system-ui,-apple-system,sans-serif}main{max-width:760px;margin:48px auto;padding:0 18px}.top{display:flex;align-items:center;gap:18px;margin-bottom:26px}.logo{width:68px;height:68px;flex:none}.eyebrow{color:var(--accent);font-size:12px;letter-spacing:.17em;font-weight:750;text-transform:uppercase}h1{margin:3px 0;font-size:29px;letter-spacing:-.04em}p{color:var(--muted);margin:4px 0}.card{background:var(--panel);border:1px solid var(--line);border-radius:18px;padding:24px;margin:16px 0;box-shadow:0 10px 30px #0002}.card h2{font-size:16px;margin:0 0 20px}.grid{display:grid;grid-template-columns:1fr 1fr;gap:18px}.small{font-size:12px;color:var(--muted);margin-bottom:6px}.value{font-weight:600;overflow-wrap:anywhere}.full{grid-column:1/-1}.tag{display:inline-flex;align-items:center;gap:7px;border-radius:999px;padding:6px 11px;background:#143c33;color:#9df3c4;font-size:12px;font-weight:700}.dot{width:7px;height:7px;background:#56df96;border-radius:50%}.hostname{background:#0d1421;border:1px solid #32445d;padding:13px 15px;border-radius:10px;color:#b6d0ff;font-family:ui-monospace,SFMono-Regular,monospace;font-size:14px;overflow-wrap:anywhere}label{display:block;font-size:13px;color:#c5d0e2;margin:16px 0 7px;font-weight:650}input,select{width:100%;border:1px solid #3a4960;border-radius:10px;padding:12px 13px;color:var(--text);background:#0c1320;font:inherit}input:disabled{opacity:.45}.hint{font-size:12px;color:var(--muted);line-height:1.5;margin-top:9px}button{margin-top:22px;border:0;background:linear-gradient(135deg,#79acff,#4b7fe9);color:#081225;padding:13px 20px;border-radius:11px;font-weight:800;font-size:14px;cursor:pointer}footer{color:#77869b;text-align:center;font-size:12px;padding:14px 0 30px}@media(max-width:560px){main{margin:24px auto}.grid{grid-template-columns:1fr}.card{padding:19px}h1{font-size:24px}}
</style></head><body><main><div class="top"><svg class="logo" viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg" aria-label="HotButton logo"><defs><linearGradient id="g" x1="0" y1="0" x2="1" y2="1"><stop stop-color="#85b7ff"/><stop offset="1" stop-color="#5573ff"/></linearGradient></defs><circle cx="50" cy="50" r="43" fill="#0b1323" stroke="url(#g)" stroke-width="7"/><circle cx="50" cy="50" r="29" fill="#233c64" stroke="#476ea7" stroke-width="2"/><circle cx="50" cy="50" r="20" fill="url(#g)"/><path d="M50 37v26M37 50h26" stroke="#dceaff" stroke-width="5" stroke-linecap="round" opacity=".85"/></svg><div><div class="eyebrow">Device control</div><h1>HotButton</h1><p>Network Setup</p></div></div>)HBHTML";
  if (msg.length()) { x += F("<section class=\"card\"><strong>"); x += msg; x += F("</strong></section>"); }
  x += F("<section class=\"card\"><h2>Device overview</h2><div class=\"grid\"><div><div class=\"small\">Device name</div><div class=\"value\">");
  x += DEVICE_DISPLAY_NAME;
  x += F("</div></div><div><div class=\"small\">Network mode</div><div class=\"value\">");
  x += storedConfig.staticEnabled ? "Static" : "DHCP";
  x += F("</div></div><div class=\"full\"><div class=\"small\">Device ID</div><div class=\"value\">");
  x += DEVICE_NAME;
  x += F("</div></div><div><div class=\"small\">Current IP</div><div class=\"value\">");
  x += ipText(curIP);
  x += F("</div></div><div><div class=\"small\">Companion pairing</div><div class=\"value\">");
  x += companionPaired ? "Paired" : "Not paired";
  x += F("</div></div><div class=\"full\"><div class=\"small\">mDNS address</div><div class=\"hostname\">");
  x += mdnsHost.length() ? mdnsHost : makeMDNSHost();
  x += F(".local</div></div></div></section><section class=\"card\"><h2>Network configuration</h2><form action=\"/save\" method=\"get\"><label>Network mode</label><select name=\"mode\"><option value=\"dhcp\"");
  if (!storedConfig.staticEnabled) x += F(" selected");
  x += F(">DHCP (automatic)</option><option value=\"static\"");
  if (storedConfig.staticEnabled) x += F(" selected");
  x += F(">Static IP</option></select><div class=\"grid\"><div><label>IP address</label><input class=\"staticField\" name=\"ip\" value=\"");
  x += ipText(cfgIP);
  x += F("\"></div><div><label>Subnet mask</label><input class=\"staticField\" name=\"subnet\" value=\"");
  x += ipText(cfgSN);
  x += F("\"></div><div><label>Gateway</label><input class=\"staticField\" name=\"gateway\" value=\"");
  x += ipText(cfgGW);
  x += F("\"></div><div><label>Companion unicast IP</label><input name=\"companion\" value=\"");
  x += ipText(companionIP);
  x += F("\"></div></div><div class=\"hint\">0.0.0.0 clears Companion IP. Changing it invalidates pairing; entering an IP does not pair the device.</div><button type=\"submit\">Save &amp; Restart Network</button></form></section><footer>HotButton · Embedded network configuration</footer></main><script>function updateMode(){let st=document.querySelector(\"[name=mode]\").value===\"static\";document.querySelectorAll(\".staticField\").forEach(e=>e.disabled=!st)}document.querySelector(\"[name=mode]\").addEventListener(\"change\",updateMode);updateMode();</script></body></html>"); return x;
}
void saveFromWeb(const String &q) {
  String mode=queryValue(q,"mode"), a=queryValue(q,"ip"), b=queryValue(q,"subnet"), c=queryValue(q,"gateway"), d=queryValue(q,"companion");
  IPAddress comp; if(!parseIPv4(d.c_str(),comp)){httpReply(setupPage("Invalid Companion IP."),"400 Bad Request");return;}
  if(mode=="static") { IPAddress ip,sn,gw; if(!parseIPv4(a.c_str(),ip)||!parseIPv4(b.c_str(),sn)||!parseIPv4(c.c_str(),gw)||isZeroIP(ip)||isZeroIP(sn)){httpReply(setupPage("Invalid static network settings."),"400 Bad Request");return;} if(!setStaticNetworkConfig(ip,sn,gw)){httpReply(setupPage("Could not save network settings."),"500 Internal Server Error");return;} }
  else if(mode=="dhcp") { if(!setDHCPNetworkConfig()){httpReply(setupPage("Could not save DHCP mode."),"500 Internal Server Error");return;} }
  else {httpReply(setupPage("Invalid mode."),"400 Bad Request");return;}
  bool ok = isZeroIP(comp) ? clearPairingConfig() :
    (sameIP(comp, companionIP) ? true : savePairingIP(comp, false));
  if(ok) connectionEstablishedThisBoot = false; if(!ok){httpReply(setupPage("Network saved, but Companion IP could not be saved."),"500 Internal Server Error");return;}
  // Redirect using a plain GET to /, never /save (which would save and restart again).
  // A new static IP must be used as the redirect destination.
  String page = F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'></head><body style='background:#0a0e16;color:#edf2fb;font-family:system-ui;padding:30px'><h1>Saved</h1><p>Settings saved. Returning to setup in 3 seconds...</p><script>setTimeout(function(){location.href='http://");
  page += makeMDNSHost();
  page += F(".local/';},3000);</script><p>If the hostname is not available, use the device IP address.</p></body></html>");
  httpReply(page); webApplyPending=true; webApplyAtMs=millis()+750;
}
void handleHTTP() {
  if(!networkOnline||!httpStarted)return; uint8_t st=getSn_SR(HTTP_SOCKET);
  if(st==SOCK_CLOSED){httpStarted=startHTTPServer();return;} if(st==SOCK_INIT){listen(HTTP_SOCKET);return;} if(st==SOCK_CLOSE_WAIT){disconnect(HTTP_SOCKET);return;} if(st!=SOCK_ESTABLISHED)return;
  uint8_t buf[1024]; int32_t n=recv(HTTP_SOCKET,buf,sizeof(buf)-1); if(n<=0)return; buf[n]=0; String r((char*)buf); int e=r.indexOf("\r\n"); if(e<0){disconnect(HTTP_SOCKET);return;} String line=r.substring(0,e);
  if(line.startsWith("GET /save?")){int q=line.indexOf('?'),sp=line.indexOf(' ',q); if(q>=0&&sp>q)saveFromWeb(line.substring(q+1,sp)); else httpReply("Bad request","400 Bad Request");}
  else if(line.startsWith("GET / ")||line.startsWith("GET /index.html "))httpReply(setupPage()); else httpReply("Not found","404 Not Found");
  // Wait for TCP TX buffer to drain before closing. The prior 1s
  // deadline could cut off the page while the browser was still receiving it.
  const uint32_t txWaitStart = millis();
  while (getSn_TX_FSR(HTTP_SOCKET) < getSn_TxMAX(HTTP_SOCKET) &&
         (uint32_t)(millis() - txWaitStart) < 3000 &&
         (getSn_SR(HTTP_SOCKET) == SOCK_ESTABLISHED ||
          getSn_SR(HTTP_SOCKET) == SOCK_CLOSE_WAIT)) delay(2);
  if (getSn_TX_FSR(HTTP_SOCKET) < getSn_TxMAX(HTTP_SOCKET))
    DEBUG_PRINTLN("HTTP TX: close with pending bytes");
  disconnect(HTTP_SOCKET);
}
void handlePendingWebApply(){if(webApplyPending&&(int32_t)(millis()-webApplyAtMs)>=0){webApplyPending=false;restartNetworkNow();}}

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

bool sendOSCOneString(const char *path, const char *value) {
  if (!networkOnline || !udpStarted || !companionPaired) return false;
  uint8_t packet[256]; size_t pos = 0;
  pos = oscWritePaddedString(packet, sizeof(packet), pos, path);
  if (pos == SIZE_MAX) return false;
  pos = oscWritePaddedString(packet, sizeof(packet), pos, ",s");
  if (pos == SIZE_MAX) return false;
  pos = oscWritePaddedString(packet, sizeof(packet), pos, value);
  if (pos == SIZE_MAX) return false;
  uint8_t destination[4]; copyIPToBytes(companionIP, destination);
  return sendto(OSC_SOCKET, packet, static_cast<uint16_t>(pos), destination, OSC_PORT, 4) == static_cast<int32_t>(pos);
}
void sendDeviceName() {
  char path[96]; snprintf(path, sizeof(path), "/%s/device/name", DEVICE_NAME);
  if (sendOSCOneString(path, DEVICE_DISPLAY_NAME)) lastNameSentMs = millis();
}

bool sendOSCOneInt(const char *path, int32_t value);
void handleConnectionCheck() {
  if (!networkOnline || !udpStarted || !companionPaired) return;
  if (connectionPhase != ConnectionPhase::CHECKING && connectionPhase != ConnectionPhase::RECOVERY &&
      connectionPhase != ConnectionPhase::CONFIRMED && connectionPhase != ConnectionPhase::RECONNECTING) return;
  const uint32_t now = millis();
  if (connectionPhase == ConnectionPhase::CHECKING && now - connectionCheckStartedMs >= CONNECTION_RECOVERY_AFTER_MS) {
    connectionPhase = ConnectionPhase::RECOVERY;
    displayMode = DisplayMode::UNPAIRED_WAIT;
  }
  // A previously confirmed connection is monitored continuously. Missing
  // acknowledgements trigger blue reconnecting, never orange boot recovery.
  if (connectionPhase == ConnectionPhase::CONFIRMED &&
      now - lastValidConnectionAckMs >= CONNECTION_ACK_TIMEOUT_MS) {
    connectionPhase = ConnectionPhase::RECONNECTING;
    connectionTokenPending = false;
    displayMode = DisplayMode::CONNECTION_WAIT;
    renderColor(COLOR_BLUE);
    lastConnectionCheckMs = now - CONNECTION_CHECK_INTERVAL_MS;
  }
  if (now - lastConnectionCheckMs < CONNECTION_CHECK_INTERVAL_MS) return;
  lastConnectionCheckMs = now;
  // Token is unique within this boot session; stale replies are ignored.
  pendingConnectionToken = (int32_t)(++connectionSequence ^ (uint32_t)micros());
  connectionTokenPending = true;
  char path[96];
  snprintf(path, sizeof(path), "/%s/connection/check", DEVICE_NAME);
  sendOSCOneInt(path, pendingConnectionToken);
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

  if (!companionPaired || connectionPhase != ConnectionPhase::CONFIRMED) return;
  if (sendOSCOneInt(path, buttonState)) {
    lastHeartbeatMs = millis();
  }
}

void handleHeartbeat() {
  if (!networkOnline || !udpStarted) {
    return;
  }

  const uint32_t now = millis();

  if (companionPaired && connectionPhase == ConnectionPhase::CONFIRMED &&
      now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
    sendHeartbeat();
  }
  if (connectionPhase == ConnectionPhase::CONFIRMED && now - lastNameSentMs >= 30000) {
    sendDeviceName(); lastNameSentMs = now;
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

  // Dedicated unicast connection acknowledgement with the exact outstanding token.
  if (pathEquals(path, "/connection/ack") && strcmp(typeTags, ",i") == 0) {
    int32_t token = 0;
    if (oscReadInt32(data, dataLen, pos, token) &&
        (connectionPhase == ConnectionPhase::CHECKING || connectionPhase == ConnectionPhase::RECOVERY ||
         connectionPhase == ConnectionPhase::CONFIRMED || connectionPhase == ConnectionPhase::RECONNECTING) &&
        companionPaired && sourceIP == companionIP && connectionTokenPending && token == pendingConnectionToken) {
      lastValidConnectionAckMs = millis();
      connectionTokenPending = false;
      if (connectionPhase != ConnectionPhase::CONFIRMED) {
        confirmCompanionConnection();
        sendLEDStateSnapshot();
      }
    }
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
      if (connectionPhase != ConnectionPhase::UNPAIRED && connectionPhase != ConnectionPhase::RECOVERY) return;
      if (savePairingIP(sourceIP)) {
        char ackPath[96];
        snprintf(ackPath, sizeof(ackPath), "/%s/pair/ack", DEVICE_NAME);
        sendOSCNoArguments(ackPath);
        confirmCompanionConnection();
        sendLEDStateSnapshot();
        DEBUG_PRINT("PAIRING: stored Companion IP ");
        DEBUG_PRINTLN(companionIP);
      }
      return;
    }

    // Optional explicit unpair command for Companion. Clear persistent pairing
    // and acknowledge via broadcast because no unicast target remains stored.
    if (pathEquals(path, "/pair/clear")) {
      // Clearing a paired device requires the local web UI or physical reset.
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

  // Custom flash timing: 3 OSC integers (milliseconds).
  if (strcmp(typeTags, ",iii") == 0 && pathEquals(path, "/led/flash/custom")) {
    int32_t onMs, offMs, fadeMs;
    if (!oscReadInt32(data, dataLen, pos, onMs) ||
        !oscReadInt32(data, dataLen, pos, offMs) ||
        !oscReadInt32(data, dataLen, pos, fadeMs)) return;
    setUserCustomFlash(onMs, offMs, fadeMs);
    sendLEDStateSnapshot();
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

    if (networkOnline && udpStarted) {
      if (connectionPhase == ConnectionPhase::CONFIRMED) {
        sendButtonEvent(stableButtonState == LOW);
      } else if (connectionPhase == ConnectionPhase::UNPAIRED || connectionPhase == ConnectionPhase::RECOVERY) {
        if (stableButtonState == LOW) {
          // Discovery is broadcast even when an old Companion IP is stored.
          const bool previousPairing = companionPaired;
          companionPaired = false;
          sendButtonEvent(true);
          companionPaired = previousPairing;
        }
      }
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
  if (networkOnline && udpStarted && connectionEstablishedThisBoot && stableButtonState == LOW) {
    sendButtonEvent(false);
  }

  setDHCPNetworkConfig();

  // The physical recovery reset must also forget the persisted Companion
  // destination. Otherwise a button paired to an unreachable/old Companion
  // IP would remain stuck in unicast mode even after network recovery.
  connectionEstablishedThisBoot = false;
  if (clearPairingConfig()) {
    DEBUG_PRINTLN("NETWORK RESET: Companion pairing cleared; broadcast mode active");
  } else {
    DEBUG_PRINTLN("NETWORK RESET ERROR: failed to persist cleared Companion pairing");
  }

  // Ignore the still-held physical button until it has been released,
  // otherwise the restart could create a second/phantom button event.
  suppressButtonUntilRelease = true;

  restartNetworkNow();

  // Distinct acknowledgement: reset was accepted, user can release now.
  resetConfirmActive = true;
  resetConfirmStartedMs = millis();
  resetConfirmPhaseChangedMs = millis();
  resetConfirmPhaseOn = true;
  renderColor(COLOR_RED);
}



// ============================================================
// SETUP
// ============================================================

void setup() {
  // USB device descriptors for normal firmware operation (not BOOTSEL).
  // Keep the existing VID/PID and USB CDC serial functionality unchanged.
  USB.disconnect();
  USB.setManufacturer("kra55k0pf");
  USB.setProduct("kra55k0pf HotButton");
  (USB.connect)(); // Avoid W6300 socket.h connect(...) macro collision

  if (debug_serial) {
    Serial.begin(115200);
  }
  delay(250);
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("=== HotButton v1.2.0 ===");

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
  initDeviceIdentity();
  loadOrCreateDeviceName();
  loadPairingConfig();
  DEBUG_PRINT("Device ID: "); DEBUG_PRINTLN(DEVICE_NAME);
  DEBUG_PRINT("Device name: "); DEBUG_PRINTLN(DEVICE_DISPLAY_NAME);
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

  // Hardware DHCP recovery: 5 s hold, only during the first
  // 30 seconds after boot. This check is fully non-blocking.
  handleBootNetworkReset();

  // Button discovery/normal events are gated by the connection phase.
  handleButton();

  // Receive OSC whenever the UDP listener is active.
  handleIncomingOSC();
  handleConnectionCheck();

  // Optional browser-based network / Companion unicast setup.
  handleHTTP();
  handleMDNS();
  handleMDNSAnnouncements();
  handlePendingWebApply();

  // Broadcast presence while the network is healthy.
  handleHeartbeat();

  // Status display has priority over user LED state.
  handleStatusDisplay();

  // User flashing runs only after status display has finished.
  handleUserFlash();

  // Keep loop cooperative.
  yield();
}
