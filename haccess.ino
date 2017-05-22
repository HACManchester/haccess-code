// Haccess v1 Access Control System
// Copyright 2015 Ben Dooks <ben@fluff.org>

#include "build.h"
#include "trigger.h"
#include "timer.h"
#include "trigger_config.h"
#include "stm32_support.h"

#include <ESP8266WiFi.h>
#include "FS.h"

// for nfc library
#include <Wire.h>
#include <SPI.h>


#if !defined(NFC_ELECHOUSE)
// nfc library
//#error "don't use this"
#include <Adafruit_PN532.h>

#else
#include <PN532_I2C.h>
#include <PN532_SPI.h>
#include <PN532.h>
#endif

// webserver stuff
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

// mqtt server

// for the display (to be moved out)
//#include <Adafruit_GFX.h>
//#include <Adafruit_PCD8544.h>

// Flash settings for the ESP07:
// QIO, 80MHz, 1M (512K SPIFFS)
// note, trying 40MHz QIO

#ifdef USE_PCD8544
extern "C" {
#include "pcd8544.h"
}

// the D/C line is set to gpio expander
// the reset line is set to gpio expander
//Adafruit_PCD8544 display = Adafruit_PCD8544(14, 13, -1, 0, -2);

#else

#define PCD8544_gotoXY(x, y) do { } while(0)
#define PCD8544_lcdPrint(str) do { } while(0)
#define PCD8544_lcdCharacter(ch) do { } while(0)
  
#endif

// IniFile library
#include "IniFile.h"

// local includes
#include "haccess.h"
#include "cards.h"
#include "config.h"
#include "gpioexp.h"
#include "urlwatch.h"
#include "mqtt.h"

// MQTT state
WiFiClient mqttWiFi;
PubSubClient mqtt(mqttWiFi);

static bool force_card_reload;

// for the moment we have some very simple scheduling informaton
// using millisecond since 'x' to check
unsigned long lastCard = 0;
unsigned long lastFile = 0;
unsigned long lsatConfig = 0;
unsigned long lastTimer = 0;
unsigned long lastMQTT = 0;
unsigned long lastSquawk = 0;

// intervals for checking the card
unsigned long fileInterval = 5000;     // check for new card file every 5seconds by default
const unsigned long configInterval = 10000;  // check for new config file every 10seconds by default
const unsigned long mqttInterval = 500;      // check for mqtt state
const unsigned long squawkInterval = 5000;   // send state 'squawk' to the mqtt

// configuration information

bool wifi_up = false;

// internal state information
bool fs_opened = false;
bool have_pn532 = true;

bool dbg_showCardRead = false;

unsigned button_count;
unsigned detect_count;


#if !defined(NFC_ELECHOUSE)
#define PN_IIC 1

// had to change the pcd code to leave clk line high
// instantiate the card reader module
#ifdef PN_IIC
// changed adafruit library to ignore reset and irq
Adafruit_PN532 nfc(-1, 12);
#else
Adafruit_PN532 nfc(14, 12, 13, 16);
#endif
//Adafruit_PN532 nfc(14, 12, 13, 2);
#endif

#if defined (NFC_ELECHOUSE)

#if 0
PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);
#else
PN532_SPI pn532spi(SPI, 0);   // v2 - spi on 0
PN532 nfc(pn532spi);
#endif

#endif

class UrlWatch *watch_cfg;

#ifdef USE_MCP
static void setup_gpioexp_mcp(void)
{
  int ret;

  Serial.println("Initialising GPIO expander");

  pinMode(15, OUTPUT);
  digitalWrite(15, LOW);
  delay(1);
  digitalWrite(15, HIGH);

  Wire.beginTransmission(MCP_IICADDR);
  Wire.write(0x00);
  ret = Wire.endTransmission();

  /* under certain conditions the GPIO expander may fail to intiialised.
   * so check the initial write to it before allowing it to continue.
   * note, makes the assumption if we get one good write, the rest will work
   */
  if (ret != 0) {
    Serial.println("ERROR: failed to talk to PCA IO expander");
    while (false) {
      ESP.wdtFeed(); delay(1);
      Wire.beginTransmission(MCP_IICADDR);
      Wire.write(0x00);
      ret = Wire.endTransmission();
    }
  }

  // set pull-ups for various pins
  gpio_exp_wr(MCP_GPPU, 0x3);   // todo - check for correct settings

  // set no input to non inverted
  gpio_exp_wr(MCP_IPOL, 0x0);

  // set the io-direction to make GP[7..4] to output
  gpio_exp_wr(MCP_IODIR, byte(0xff ^ 0xF0));

  Serial.print("MCP_IODIR=");
  Serial.println(gpio_exp_rd(MCP_IODIR));

  // set output values
  gpio_exp_initout(0x00);       // start with all off

  // interrupt settings

  // set the "normal values"
  gpio_exp_wr(MCP_DEFVAL, 0x03 | 0x08); // todo - check if these are correct

  // set both buttons to interrupt on change
  gpio_exp_wr(MCP_GPINTEN, 0x3);

  // set interrupt to any of the input pins
  gpio_exp_wr(MCP_INTCON, 0x0f);

  Serial.print("MCP_INTCON=");
  Serial.println(gpio_exp_rd(MCP_INTCON));

  Serial.print("MCP_IODIR=");
  Serial.println(gpio_exp_rd(MCP_IODIR));
}
#else
static void setup_gpioexp_mcp(void)
{
  Serial.println("MCP IO expander not used");
}
#endif

static void read_wifi_config(void)
{
  File f = SPIFFS.open("/wifi.txt", "r");

  if (!f) {
    Serial.println("cannot find wifi configration file");
    return;
  }

  String r;

  r = f.readStringUntil('\n');
  Serial.print("ESSID ");
  Serial.println(r);
  cfg.wifi_ssid = r.c_str();

  r = f.readStringUntil('\n');
  Serial.print("Password ");
  Serial.println(r);
  cfg.wifi_pass = r.c_str();

  f.close();
}

static void start_wifi(void)
{
  WiFi.begin(cfg.wifi_ssid, cfg.wifi_pass);
}

static void show_ids(void)
{
  Serial.print("ESP ChipID ");
  Serial.println(ESP.getChipId(), HEX);
  Serial.print("ESP SDK Version ");
  Serial.println(ESP.getSdkVersion());
  Serial.print("ESP Boot version ");
  Serial.println(ESP.getBootVersion());
  Serial.print("Free heap size ");
  Serial.println(ESP.getFreeHeap());
}

static void init_nfc(void)
{
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();

  Serial.print("NFC version info is ");
  Serial.println(versiondata, HEX);

  if (! versiondata) {
    have_pn532 = false;
    Serial.println("Didn't find PN53x board");
    return;
  }

  if ((versiondata >> 24) != 0x32) {
    Serial.println("Device is not PN532?");
  }

  nfc.setPassiveActivationRetries(1);
  nfc.SAMConfig();
}

static void check_card_list(void)
{
  File f;

  f = SPIFFS.open("/cardlist", "r");
  if (f) {
    Serial.println("opening card list from flash");
    readCardList(f);
  } else {
    Serial.println("no card list at startup");
  }
  f.close();
}

#ifdef USE_PCD8544
static PCD8544_Settings pcd8544_settings;

static void setupDisplay(void)
{
  pcd8544_settings.lcdVop = 0xB1;
  pcd8544_settings.tempCoeff = 0x04;
  pcd8544_settings.biasMode = 0x14;
  pcd8544_settings.inverse = false;

  pcd8544_settings.resetPin = 127;        // reset handled at init time.
  pcd8544_settings.scePin = 127;          // sce pin isn't being used on v2 boards

  pcd8544_settings.dcPin = 0;
  pcd8544_settings.sdinPin = 13;
  pcd8544_settings.sclkPin = 14;

  Serial.println("Initialising display");
  PCD8544_init(&pcd8544_settings);
  Serial.println("Initialised display");

  gpio_exp_setgpio(6, 1);   // turn backlight on

  if (true) {
     Serial.println("displaying serial boot message");
     PCD8544_gotoXY(0, 0);
     PCD8544_lcdPrint("HACCESS 1 BOOT");
  }  
}
#else
static void setupDisplay(void)
{
}
#endif /* USE_PCD8544 */

static void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  class trigger *trig;
  char *value = (char *)payload;

  Serial.printf("topic '%s', payload '%s', lenght %d\n", topic, payload, length);
  // nothing we really care about here at the moment

  if (length < 1) {
    return;     // ignore events with zero data.
  }

  trig = trigger_find(topic);
  if (trig) {
    if (!trig->is_input()) {
      Serial.printf("topic '%s' is not input.\n", topic);
      return;
    }

    if (strcmp(value, "1") == 0 ||
        strcmp(value, "on") == 0) {
      trig->new_state(true);
    } else if (strcmp(value, "0") == 0 ||
               strcmp(value, "off") == 0) {
      trig->new_state(false);
    } else if (strcmp(value, "signal") == 0) {
      trig->new_state(true);
      trig->new_state(false);
    } else {
      // did not understand.

      Serial.printf("mqtt: topic %s value %s not understood\n",
      			   topic, payload);
    }
  }
}

static void setup_mqtt(void)
{
  char tmp[128];

  cfg.mqtt_server = get_cfg_str("mqtt", "server");
  if (!cfg.mqtt_server) {
    Serial.println("no mqtt server specified");
    return;
  }

  cfgfile.clearError();
  if (cfgfile.getValue("mqtt", "port", tmp, sizeof(tmp), cfg.mqtt_port)) {
    // ok
    if (cfgfile.getError() != cfgfile.errorNoError)
      Serial.print("read true, but error?");
    Serial.printf("mqtt port %d\n", cfg.mqtt_port);
  } else {
    cfg.mqtt_port = 1883;
  }

  mqtt.setServer(cfg.mqtt_server, cfg.mqtt_port);
  mqtt.setCallback(mqtt_callback);
}

static void open_config_file(void)
{
  File cfg;

  cfg = SPIFFS.open("/config.ini", "r");
  if (!cfg) {
    Serial.println("ERROR: failed to open configuration file");
    return;
  }

  set_config_file(cfg);
}

static bool config_wdt = false;

#define WDT_ADDR  (0x50)

// pet the external watchdog (if it exists)
static void process_wdt(void)
{
  if (!config_wdt)
    return;
  wdt_pet();
}


char cfgbuff[128];

static char *get_cfg_str(const char *sect, const char *key)
{
  char *ret;

  if (!cfgfile.getValue(sect, key, cfgbuff, sizeof(cfgbuff))) {
    Serial.printf("config: failed to find %s.%s\n", sect, key);
    return NULL;
  }

  ret = strdup(cfgbuff);
  if (!ret) {
    Serial.printf("ERROR: no memory for %s.%s (%s)\n", sect, key, cfgbuff);
  }

  return ret;
}

static void setup_card_list(void)
{
  const char *section = "cards";
  char tmp[128];

  if (!cfgfile.getValue(section, "use", tmp, sizeof(tmp), cfg.en_cards))
    goto parse_err;

  if (!cfgfile.getValue(section, "fetch", tmp, sizeof(tmp), cfg.en_cards_fetch))
    goto parse_err;

  if (cfg.en_cards_fetch) {
    if (!cfgfile.getValue(section, "update", tmp, sizeof(tmp), cfg.en_cards_update))
      goto parse_err;

    if (!cfgfile.getValue(section, "watch", tmp, sizeof(tmp), cfg.en_cards_watch))
      goto parse_err;

    if (!cfgfile.getValue(section, "interval", tmp, sizeof(tmp), cfg.card_interval))
      cfg.card_interval = 600;
    fileInterval = cfg.card_interval * 1000;

    cfg.card_host = get_cfg_str(section, "host");
    cfg.card_url = get_cfg_str(section, "url");
    cfg.card_auth = get_cfg_str(section, "authinfo");
    if (!cfg.card_host || !cfg.card_url)
      goto parse_err;

    if (cfg.en_cards_watch) {
      watch_cfg = new UrlWatch(cfg.card_host, 80, cfg.card_url);
      if (!watch_cfg) {
        cfg.en_cards_fetch = false;
        cfg.en_cards_update = false;
        Serial.println("no memory for url watch");
        goto parse_err;
      }
    }
  }

  return;

parse_err:
  Serial.println("ERROR: failed to parse cards section");
}

void setup() {
  //Wire.begin(5, 4);
  Wire.begin(4, 5);
  Serial.begin(115200);
  Serial.println("ESP8266 Haccess node");

  twi_setClock(100000);
  Serial.println("Setting i2c clock stretch to 500uS");
  twi_setClockStretchLimit(7000);  // think this is 700uS

  // ensure the pins for spi are configured for correct mode
  pinMode(14, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(0, OUTPUT);
  pinMode(12, INPUT);   // IRQ from card reader
  pinMode(2, INPUT);    // irq pin

  // set the pins
  digitalWrite(15, 0);
  digitalWrite(14, 0);
  digitalWrite(0, 1);

  // show the chip and sdk version
  show_ids();
  setup_gpioexp();
  process_wdt();

  //gpio_exp_setgpio(5, 0); // set the display nCS=0
  //gpio_exp_setgpio(6, 1); // set the display BL=1 (on)
  setupDisplay();

  Serial.println("MAC " + WiFi.macAddress());

  fs_opened = SPIFFS.begin();
  if (!fs_opened) {
    Serial.println("Failed to open SPIflash");
  } else {
    Serial.println("SPIFFS started");
  }

  Serial.println("open config file");
  open_config_file();
  Serial.println("start wdt");
  process_wdt();

  Serial.println("wifi startup");
  read_wifi_config();
  start_wifi();

  Serial.println("preparing mqtt");
  setup_mqtt();
  init_nfc();

  setup_triggers();

  setup_card_list();
  check_card_list();
  process_wdt();

#if defined(PN_IIC)
  Serial.println("PN532 is IIC connected");
#else
  Serial.println("PN532 is SPI connected");
#endif

  Serial.print("WiFi connected, IP ");
  Serial.println(WiFi.localIP());

  Serial.printf("Setup done, free heap size %d\n", ESP.getFreeHeap());

#define dump_cfg(__cfg) Serial.printf("CFG: %s = %d\n", #__cfg, cfg.__cfg)
  if (true) {
    dump_cfg(en_rfid);
    dump_cfg(en_mqtt);
    dump_cfg(en_cards);
    dump_cfg(en_cards_watch);
    dump_cfg(en_cards_fetch);
    dump_cfg(en_cards_update);
    dump_cfg(rfid_interval);
    dump_cfg(card_interval);
  }
}


static void checkForNewFiles(void)
{
  if (watch_cfg->upToDate() == URL_CHANGED) {
    Serial.println("configuration changed\n");
    copyCardList(watch_cfg->getHost(), watch_cfg->getPort(), watch_cfg->getUrl(), NULL);
    Serial.println("configuration fetched\n");
    // todo - announce the new load?
  }
}

static void fetchNewFiles(void) {
  copyCardList(cfg.card_host, 80, cfg.card_url, cfg.card_auth);
}

static void show_known_card(class CardInfo *info, uint8_t *uid, uint8_t uidLength)
{
  PCD8544_gotoXY(0, 3);
  PCD8544_lcdPrint("Hello");
  PCD8544_gotoXY(0, 4);
  PCD8544_lcdPrint(info->logname.c_str());
}

static void show_unknown_card(class CardInfo *info, uint8_t *uid, uint8_t uidLength)
{
  char tmp[4];

  PCD8544_gotoXY(0, 3);
  PCD8544_lcdPrint("Unknown card");
  PCD8544_gotoXY(0, 4);

  for (uint8_t i = 0; i < uidLength; i++) {
    sprintf(tmp, "%02x", uid[i]);
    PCD8544_lcdPrint(tmp);
    if (i < (uidLength - 1)) {
      PCD8544_lcdPrint(":");
    }
  }
}

static int card_ok_count;
static bool show_card = true;

static void checkForCard(bool show_fail)
{
  boolean success;
  uint8_t uid[8] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;  // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  class CardInfo info;
  bool known_card = false;

  if (show_fail) Serial.println("Reading card");
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  //Serial.println("Read card");

  if (success) {
    detect_count = 5;
    if (show_card) {
      Serial.println("Found a card!");
      Serial.print("UID Length: ");
      Serial.print(uidLength, DEC);
      Serial.println(" bytes");

      Serial.print("UID Value: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        Serial.print(" 0x"); Serial.print(uid[i], HEX);
      }
      Serial.println("");
    }

    known_card = lookupCard(&info, uid, uidLength);
    if (known_card) {
      Serial.println("Found card " + info.logname);
      show_known_card(&info, uid, uidLength);
      card_ok_count = 3;
    } else {
      Serial.println("Found unknown card");
      show_unknown_card(&info, uid, uidLength);
    }

    mqtt.publish(known_card ? "haccess/knownrfid" : "haccess/unknownrfid", uid, uidLength);
  } else {
    if (show_fail)
      Serial.println("no card");
  }
}

static unsigned gpio_get(void)
{
  unsigned ret = 0;

  ret |= (gpio_read(GPIO_IN_R)) ? 1 : 0;
  ret |= (gpio_read(GPIO_IN_G)) ? 2 : 0;
  ret |= (gpio_read(GPIO_IN_USER)) ? 4 : 0;
  return ret;
}

static unsigned old_gpio = 0xff;

static void checkGPIOs(void)
{
  if (true) {
    unsigned gpio = gpio_get();

    gpio &= 7;
    if (gpio == old_gpio)
      return;

    Serial.printf("gpio change %x to %x\n", old_gpio, gpio);
    old_gpio = gpio;

    if (false)
      return;
   
    in_button1->new_state((gpio & 1) == 0);
    in_button2->new_state((gpio & 2) == 0);
    in_opto->new_state((gpio & 4) == 0);

    PCD8544_gotoXY(64 + 5, 0);
    PCD8544_lcdCharacter((gpio & 1) ? 'x' : '-');
    PCD8544_lcdCharacter((gpio & 2) ? 'x' : '-');
    PCD8544_lcdCharacter((gpio & 4) ? 'x' : '-');

    // test: mirror to port expander gpio_exp_setgpio(7, (gpio & 4) ? false : true);

    button_count = 5;
  } else {
  }
}

// serial information (mostly for debug)
static void serial_interaction(void)
{
  int byte = Serial.read();

  switch (byte) {
    case 'r':
      Serial.println("Rebooting...");
      ESP.restart();
      break;

    case 'R':
      checkForCard(true);
      break;

    case 'l':
      force_card_reload = true;
      break;

    case 'S':
      Serial.println("MAC " + WiFi.macAddress());
      Serial.println(WiFi.localIP());  
      Serial.printf("Heap free %d bytes\n", ESP.getFreeHeap());
      break;

    case 'O':
      card_ok_count = 3;
      break;

    case 'h':
    case 'H':
      Serial.println("Hello");
      break;

    case 'b':
      //gpio_exp_setgpio(6, 0);
      gpio_set(GPIO_OUT_BL, 0);
      break;

    case 'B':
      //gpio_exp_setgpio(6, 1);
      gpio_set(GPIO_OUT_BL, 1);
      break;

    case 'G':
      //Serial.printf("GPIO state %x, IODIR=%x, INTF=%x PU=%x\n", gpio_exp_rd(MCP_GPIO), gpio_exp_rd(MCP_IODIR), gpio_exp_rd(MCP_INTF), gpio_exp_rd(MCP_GPPU));
      break;

    case 'p':
    case 'P':
      Serial.println("@goto 0,0");
      PCD8544_gotoXY(0, 0);
      Serial.println("@print");
      PCD8544_lcdPrint("HACCESS 1 Proto");
      Serial.println("@printed");
      break;
  }
}

static bool timeTo(unsigned long *last, unsigned long interval, unsigned long curtime)
{
  if ((curtime - *last) >= interval) {
    *last = curtime;
    return true;
  }

  return false;
}

// display stuff on the screen, such as the wifi state
static void runDisplay(void)
{
  unsigned long curtime = millis();
  unsigned secs = curtime / 1000;
  unsigned mins = secs / 60;
  unsigned sr = secs % 60;
  char ct[16];

  if (detect_count || button_count) {
    bool on;

    if (detect_count)
      detect_count--;
    if (button_count)
      button_count--;

    on = (button_count || detect_count);
    gpio_set(GPIO_OUT_BL, on ? 1 : 0);

    if (!on) {
      // clear the card lcd area
      PCD8544_gotoXY(0, 3);
      PCD8544_lcdPrint("            ");
      PCD8544_gotoXY(0, 4);
      PCD8544_lcdPrint("            ");
      PCD8544_gotoXY(0, 5);
      PCD8544_lcdPrint("            ");
      // turn off the backlight
      gpio_set(GPIO_OUT_BL, 0);
    }
  }

  // print the time
  PCD8544_gotoXY(0, 0);
  sprintf(ct, "%02d:%02d", mins, sr);
  Serial.println(ct);
  PCD8544_lcdPrint(ct);
}

static void setWifiState(bool up)
{
  char wifi_str[2];

  if (up) {
    Serial.println("WiFi up");
    Serial.print(" IP ");
    Serial.println(WiFi.localIP());
    mqtt.publish("wifiup", "%ld", millis());
  } else {
    Serial.println("WiFi down");
  }

  wifi_str[0] = up ? 0x80 : 'x';
  wifi_str[1] = 0x0;
  PCD8544_gotoXY(64, 0);
  PCD8544_lcdPrint(wifi_str);

}

static void sayHello(void)
{
  unsigned long curtime = millis();
  char uptime[32];

  if (!mqtt.connected())
    return;

  sprintf(uptime, "%ld", curtime / 1000);
  mqtt.publish("haccess/uptime", uptime);
}

static bool mqtt_known = false;

static void processMqtt(void)
{
  if (!cfg.en_mqtt)
    return;

  if (!mqtt.connected()) {
    mqtt_known = false;
    Serial.println("MQTT not connected");
    if (!wifi_up)
      return;
    if (!mqtt.connect("HACCESSNode"))
      return;
    // re-subscribe here if necessary
    Serial.println("MQTT connection completed");
  } else {
    if (!mqtt_known) {
      Serial.println("MQTT connected");
      mqtt_known = true;
    }
    mqtt.loop();
  }
}

static void publish_state(void)
{
  static char data[32];
  if (!mqtt_known || !mqtt.connected())
    return;

  snprintf(data, sizeof(data), "%ld", ESP.getFreeHeap());
  mqtt.publish("haccess/freemem", data);
}

// main code loop
void loop() {
  unsigned long curtime;

  if (WiFi.status() != WL_CONNECTED) {
    if (wifi_up) {
      wifi_up = false;
      setWifiState(wifi_up);
    }
  } else {
    if (!wifi_up) {
      wifi_up = true;
      processMqtt();
      setWifiState(wifi_up);
    }
  }

  curtime = millis();

  if (timeTo(&lastCard, cfg.rfid_interval, curtime) && cfg.en_rfid && have_pn532) {
    //Serial.println("card check");
    checkForCard(false);
  }

  if ((timeTo(&lastFile, fileInterval, curtime) && cfg.en_cards_update) || force_card_reload) {
    force_card_reload = false;
    if (cfg.en_cards_watch)  
      checkForNewFiles();
    else
      fetchNewFiles();
  }
  
  if (timeTo(&lastTimer, 1000, curtime)) {
    runDisplay();
    processMqtt();
    sayHello();
    process_wdt();

    //gpio_exp_setgpio(7, card_ok_count > 0 ? true : false);
    gpio_set(GPIO_OUT_OPTO, card_ok_count > 0 ? 1 : 0);
    if (card_ok_count > 0)
      card_ok_count--;
  }

  if (timeTo(&lastSquawk, squawkInterval, curtime) && false) {
    publish_state();
  }

  // always check gpios through the loop
  checkGPIOs();

  if (Serial.available() > 0) {
    serial_interaction();
  }

  timer_sched(curtime);
}

