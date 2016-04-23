// Haccess v1 Access Control System
// Copyright 2015 Ben Dooks <ben@fluff.org>

#include "trigger.h"
#include "timer.h"

#include <ESP8266WiFi.h>
#include "FS.h"

// for nfc library
#include <Wire.h>
#include <SPI.h>

#define NFC_ELECHOUSE

#if !defined(NFC_ELECHOUSE)
// nfc library
#include <Adafruit_PN532.h>

#else
#include <PN532_I2C.h>
#include <PN532.h>
#endif

// webserver stuff
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

// mqtt server
#include <PubSubClient.h>

// for the display (to be moved out)
//#include <Adafruit_GFX.h>
//#include <Adafruit_PCD8544.h>

// Flash settings for the ESP07:
// QIO, 80MHz, 1M (512K SPIFFS)

extern "C" {
#include "pcd8544.h"
}

// the D/C line is set to gpio expander
// the reset line is set to gpio expander
//Adafruit_PCD8544 display = Adafruit_PCD8544(14, 13, -1, 0, -2);

// local includes
#include "haccess.h"
#include "cards.h"
#include "gpioexp.h"
#include "urlwatch.h"

// IniFile library
#include "IniFile.h"

// configuration file
File cfg_file;
IniFile cfgfile;

struct config cfg = {
  .en_rfid = true,
  .en_mqtt = false,
  .en_cards = false,
  .en_cards_fetch = false,
  .rfid_interval = 250,
  .wifi_ssid = "Hackspace",
  .wifi_pass = "T3h4x0rZ",
  .card_host = NULL,
  .card_url = NULL,
  .mqtt_server = NULL,
  .mqtt_port = 1883,
};

// MQTT state
WiFiClient mqttWiFi;
PubSubClient mqtt(mqttWiFi);

// configuration information

bool wifi_up = false;

// internal state information
bool fs_opened = false;

bool dbg_showCardRead = true;

unsigned button_count;
unsigned detect_count;

// triggers

class input_trigger in_rfid;
class input_trigger in_rfid_auth;

class input_trigger in_button1;
class input_trigger in_button2;
class input_trigger in_opto;

class output_trigger out_opto;

#if !defined(NFC_ELECHOUSE)
#define PN_IIC 1

// had to change the pcd code to leave clk line high
// instantiate the card reader module
#ifdef PN_IIC
// changed adafruit library to ignore reset and irq
Adafruit_PN532 nfc(-1, -1);
#else
Adafruit_PN532 nfc(14, 12, 13, 16);
#endif
//Adafruit_PN532 nfc(14, 12, 13, 2);
#endif

#if defined (NFC_ELECHOUSE)

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

#endif

class UrlWatch *watch_cfg;

void setup_gpioexp(void)
{
  int ret;

  Serial.println("Initialising GPIO expander");

  pinMode(15, OUTPUT);
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
    while (true) {
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

  // set output values
  gpio_exp_initout(0x00);       // start with all off

  // interrupt settings

  // set the "normal values"
  gpio_exp_wr(MCP_DEFVAL, 0x03 | 0x08); // todo - check if these are correct

  // set both buttons to interrupt on change
  gpio_exp_wr(MCP_GPINTEN, 0x3);

  // set interrupt to any of the input pins
  gpio_exp_wr(MCP_INTCON, 0x0f);
}

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
  Serial.print("Free heap size");
  Serial.println(ESP.getFreeHeap());
}

static void init_nfc(void)
{
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();

  Serial.print("NFC version info is ");
  Serial.println(versiondata, HEX);

  if (! versiondata) {
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

static PCD8544_Settings pcd8544_settings;

void setupDisplay(void)
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
}

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
      // did not understand //
    }
  }
}

static void setup_mqtt(void)
{
  char tmp[128];

  cfg.mqtt_server = get_cfg_str("mqtt", "server");
  if (!cfg.mqtt_server) {
    // todo?
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

  cfg_file = cfg;
  cfgfile.setFile(cfg);
}

static bool config_wdt = true;

#define WDT_ADDR  (0x50)

// pet the external watchdog (if it exists)
static void process_wdt(void)
{
  if (!config_wdt)
    return;

  Wire.beginTransmission(WDT_ADDR);
  Wire.write(0x29);
  Wire.endTransmission();
}

/*
// look for a time-specification, in text format and return time in millis
// for example:
// 10c  => 10 centi-seconds
/  10s  => 10 seconds
// 10m  => 10 minutes
// 10h  => 10 hours
// is it worth having days in here?
*/

static bool parse_time(char *buff, unsigned long *result)
{
  unsigned long tmp;
  char *ep = NULL;

  tmp = strtoul(buff, &ep, 10);
  if (ep == buff)
    return false;

  *result = tmp;
  if (ep[0] < 32)
    return true;

  switch (ep[0]) {
    case 's':
      tmp *= 1000;
      break;

    case 'm':
      if (ep[1] != 's')
        tmp *= 1000 * 60;
      break;

    case 'c':
      tmp *= 10;
      break;

    case 'h':
      tmp *= 1000 * 60 * 60;
      break;

    default:
      return false;
  }

  *result = tmp;
  return true;
}

static void read_dependency_srcs(class trigger *target, const char *section, char *pfx)
{
  class trigger *src;
  char name[20];
  char tmp[64];
  int nr;

  for (nr = 0; nr < 99; nr++) {
    sprintf(tmp, "%s%d", pfx, nr);
    if (!cfgfile.getValue(section, name, tmp, sizeof(tmp)))
      break;


    src = get_trig(tmp);
    if (src) {
      target->add_dependency(src);
    }
  }
}

static bool read_trigger_timer(class timer_trigger *tt, const char *section, char *buff, int buff_sz)
{
  unsigned long time;

  tt->set_length(1000UL);   // default is 1sec

  if (cfgfile.getValue(section, "time", buff, buff_sz)) {
    if (!parse_time(buff, &time))
      return false;

    tt->set_length(time);
  }

  return true;
}

static void read_trigger(const char *section)
{
  class trigger *trig;
  char tmp[64];
  bool bv = false;

  if (!cfgfile.getValue(section, "type", tmp, sizeof(tmp)))
    goto parse_err;

  if (strcmp(tmp, "sr") == 0) {
    trig = new sr_trigger();
  } else if (strcmp(tmp, "or") == 0) {
    trig = new or_trigger();
  } else if (strcmp(tmp, "and") == 0) {
    trig = new and_trigger();
  } else if (strcmp(tmp, "not") == 0) {
    trig = new not_trigger();
  } else if (strcmp(tmp, "timer") == 0) {
    class timer_trigger *tt = new timer_trigger();

    if (tt) {
      if (!read_trigger_timer(tt, section, tmp, sizeof(tmp)))
        goto parse_err;
    }
    trig = tt;
  } else
    goto parse_err;

  if (!trig) {
    Serial.printf("no memory for trigger\n");
    return;
  }

  // do any standard trigger parsing that's common to all triggers

  if (cfgfile.getValue(section, "default", tmp, sizeof(tmp), bv)) {
    trig->new_state(bv);
  }

  if (cfgfile.getValue(section, "topic", tmp, sizeof(tmp))) {
    // todo - attach to the mqtt handler?
  }

  if (cfgfile.getValue(section, "expires", tmp, sizeof(tmp))) {
    unsigned long exptime;

    // create a timer that then goes and un-sets the given
    // trigger.
    if (parse_time(tmp, &exptime)) {
      class timer_trigger *tt = new timer_trigger();
      class forward_trigger *ft = new forward_trigger();

      if (!tt || !ft)
        goto parse_err;

      tt->set_name("internal");
      tt->set_length(exptime);
      tt->set_edge(true, true);
      tt->add_dependency(trig);
      ft->set_name("internal");
      ft->add_dependency(tt);
      ft->set_target(trig);
    } else {
      goto parse_err;
    }
  }

  // note, dependency information can be handled elsewhere
  read_dependency_srcs(trig, section, "source");

  return;

parse_err:
  Serial.printf("failed parsing '%s'\n", section);
}

static class trigger *get_trig(char *name)
{
    class trigger *trig = trigger_find(name);

    if (!trig)
      Serial.printf("ERROR: failed to find '%s'\n", name);
    return trig;
}

class trigger *cfg_lookup_trigger(const char *section, char *name)
{
    class trigger *trig;
    char tmp[64];

    if (!cfgfile.getValue(section, name, tmp, sizeof(tmp)))
      return NULL;

    return get_trig(tmp);
}

static void read_dependency(const char *section)
{
  class trigger *target = cfg_lookup_trigger(section, "target");
  class trigger *src;
  char tmp[64];
  int nr;

  if (!target)
    return;

  read_dependency_srcs(target, section, "source");
  // todo - if set/reset, add set/reset dependencies
}

static String get_section(String line)
{
  int end = line.indexOf(']');

  if (end > 1)
    return line.substring(1, end-1);
  return "";
}

static void read_triggers(void)
{
  File f = cfg_file;
  String line;
  uint32_t pos = 0;

  if (!f)
    return;

  while (true) {
    if (!f.seek(pos, SeekSet)) {
      Serial.println("read_triggers: failed seek\n");
      break;
    }

    line = f.readStringUntil('\n');
    // is line null if this fails?

    if (line.startsWith("[logic") ||
        line.startsWith("[input") ||
        line.startsWith("[output")) {
      String section = get_section(line);
      read_trigger(section.c_str());
    }
    pos += line.length();
  }
}

static void read_depends(void)
{
  File f = cfg_file;
  String line;
  uint32_t pos = 0;

  if (!f)
    return;

  while (true) {
    if (!f.seek(pos, SeekSet)) {
      Serial.println("read_triggers: failed seek\n");
      break;
    }

    line = f.readStringUntil('\n');
    // is line null if this fails?

    if (line.startsWith("[dependency")) {
      String section = get_section(line);
      read_dependency(section.c_str());
    }
    pos += line.length();
  }
}

static void dump_trig_dep(class trigger *trig)
{
  Serial.printf(" Dep %s: %d\n", trig->get_name(), trig->get_state());
}

static void dump_trigger(class trigger *trig)
{
  Serial.printf("Trigger %s: %d\n", trig->get_name(), trig->get_state());
  trig->run_depends(dump_trig_dep);
  Serial.println("");
}

static void setup_triggers(void)
{
  in_rfid.set_name("input/rfid");
  in_rfid_auth.set_name("input/rfid/auth");

  in_button1.set_name("input/button1");
  in_button2.set_name("input/button2");
  in_opto.set_name("input/opto");

  out_opto.set_name("output/opto");
  // todo - set output actions.

  /* we run through the configuration files in several passes. As we need
   * unique section names, we need to work through until weve read all
   * the lines */

  read_triggers();
  read_depends();

  if (true) {
    trigger_run_all(dump_trigger);
  }
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
    cfg.card_host = get_cfg_str(section, "host");
    cfg.card_url = get_cfg_str(section, "url");
    if (!cfg.card_host || !cfg.card_url)
      goto parse_err;

    watch_cfg = new UrlWatch(cfg.card_host, 80, cfg.card_url);
    if (!watch_cfg) {
      Serial.println("no memory for url watch");
      goto parse_err;
    }
  }

  return;

parse_err:
  Serial.println("ERROR: failed to parse cards section");
}

void setup() {
  Wire.begin(4, 5);
  Serial.begin(115200);

  // ensure the pins for spi are configured for correct mode
  pinMode(14, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(0, OUTPUT);

  pinMode(16, OUTPUT);  // WS LED

  pinMode(12, INPUT);   // IRQ from card reader
  pinMode(2, INPUT);    // irq pin

  // set the pins
  digitalWrite(15, 0);
  digitalWrite(14, 0);
  digitalWrite(16, 0);

  // show the chip and sdk version
  show_ids();
  setup_gpioexp();
  process_wdt();

  gpio_exp_setgpio(5, 0); // set the display nCS=0
  gpio_exp_setgpio(6, 1); // set the display BL=1 (on)
  setupDisplay();

  Serial.println("MAC " + WiFi.macAddress());

  fs_opened = SPIFFS.begin();
  if (!fs_opened) {
    Serial.println("Failed to open SPIflash");
  }

  open_config_file();
  process_wdt();

  read_wifi_config();
  start_wifi();

  setup_mqtt();
  init_nfc();

  setup_triggers();

  setup_card_list();
  check_card_list();
  process_wdt();

#if defined(PN_IIC) || defined(NFC_ELECHOUSE)
  Serial.println("PN532 is IIC connected");
#else
  Serial.println("PN532 is SPI connected");
#endif

  Serial.print("WiFi connected, IP ");
  Serial.println(WiFi.localIP());
}


static void checkForNewFiles(void)
{
  if (watch_cfg->upToDate() == URL_CHANGED) {
    Serial.println("configuration changed\n");
    copyCardList(watch_cfg->getHost(), watch_cfg->getPort(), watch_cfg->getUrl());
    Serial.println("configuration fetched\n");
  }
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

static void checkForCard(void)
{
  boolean success;
  uint8_t uid[8] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;  // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  class CardInfo info;

  //Serial.println("Reading card");
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  //Serial.println("Read card");mq

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

    if (lookupCard(&info, uid, uidLength)) {
      Serial.println("Found card " + info.logname);
      show_known_card(&info, uid, uidLength);
      card_ok_count = 3;
    } else {
      Serial.println("Found unknown card");
      show_unknown_card(&info, uid, uidLength);
    }
  } else {
    //Serial.println("no card");
  }
}

static unsigned old_gpio = 0xff;

static void checkGPIOs(void)
{
  if (true) {
    unsigned gpio = gpio_exp_rd(MCP_GPIO);

    gpio &= 7;
    if (gpio == old_gpio)
      return;

    Serial.printf("gpio change %x to %x\n", old_gpio, gpio);

    in_button1.new_state((gpio & 1) != 0);
    in_button2.new_state((gpio & 2) != 0);
    in_opto.new_state((gpio & 4) != 0);

    PCD8544_gotoXY(64 + 5, 0);
    PCD8544_lcdCharacter((gpio & 1) ? 'x' : '-');
    PCD8544_lcdCharacter((gpio & 2) ? 'x' : '-');
    PCD8544_lcdCharacter((gpio & 4) ? 'x' : '-');
    old_gpio = gpio;

    // test: mirror to port expander gpio_exp_setgpio(7, (gpio & 4) ? false : true);

    button_count = 5;
  } else {

    if ((gpio_exp_rd(MCP_GPIO) & (0x3)) != 0x3)
      gpio_exp_setgpio(7, false);
    else
      gpio_exp_setgpio(7, true);;
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

    case 'h':
    case 'H':
      Serial.println("Hello");
      break;

    case 'b':
      gpio_exp_setgpio(6, 0);
      break;

    case 'B':
      gpio_exp_setgpio(6, 1);
      break;

    case 'G':
      Serial.printf("GPIO state %x, IODIR=%x, INTF=%x PU=%x\n", gpio_exp_rd(MCP_GPIO), gpio_exp_rd(MCP_IODIR), gpio_exp_rd(MCP_INTF), gpio_exp_rd(MCP_GPPU));
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


// for the moment we have some very simple scheduling informaton
// using millisecond since 'x' to check
unsigned long lastCard = 0;
unsigned long lastFile = 0;
unsigned long lsatConfig = 0;
unsigned long lastTimer = 0;
unsigned long lastMQTT = 0;

// intervals for checking the card
const unsigned long fileInterval = 5000;     // check for new card file every 5seconds by default
const unsigned long configInterval = 10000;  // check for new config file every 10seconds by default
const unsigned long mqttInterval = 500;      // check for mqtt state

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
    gpio_exp_setgpio(6, on ? 1 : 0);

    if (!on) {
      // clear the card lcd area
      PCD8544_gotoXY(0, 3);
      PCD8544_lcdPrint("            ");
      PCD8544_gotoXY(0, 4);
      PCD8544_lcdPrint("            ");
      PCD8544_gotoXY(0, 5);
      PCD8544_lcdPrint("            ");
      // turn off the backlight
      gpio_exp_setgpio(6, 0);
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

  sprintf(uptime, "%ld", curtime / 1000);
  mqtt.publish("haccess/uptime", uptime);
}

static bool mqtt_known = false;

static void processMqtt(void)
{
  if (true)
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
      setWifiState(wifi_up);
    }
  }

  curtime = millis();

  if (timeTo(&lastCard, cfg.rfid_interval, curtime) && cfg.en_rfid)
    checkForCard();

  if (timeTo(&lastFile, fileInterval, curtime) && false)
    checkForNewFiles();

  if (timeTo(&lastTimer, 1000, curtime)) {
    runDisplay();
    processMqtt();
    sayHello();
    process_wdt();

    gpio_exp_setgpio(7, card_ok_count > 0 ? true : false);
    if (card_ok_count > 0)
      card_ok_count--;
  }

  // always check gpios through the loop
  checkGPIOs();

  if (Serial.available() > 0) {
    serial_interaction();
  }

  timer_sched(curtime);
}

