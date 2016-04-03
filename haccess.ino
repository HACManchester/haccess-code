// Haccess v1 Access Control System
// Copyright 2015 Ben Dooks <ben@fluff.org>

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
#include "cards.h"
#include "gpioexp.h"
#include "urlwatch.h"

// IniFile library
#include "IniFile.h"

// configuration file
IniFile cfgfile;

// MQTT state
WiFiClient mqttWiFi;
PubSubClient mqtt(mqttWiFi);

const char *mqtt_server;    // copy of servername

// configuration information

const char *ssid_def = "Hackspace";
const char *pass_def = "T3h4x0rZ";

String ssid = String(ssid_def);
String pass = String(pass_def);

bool wifi_up = false;

// internal state information
bool fs_opened = false;

bool dbg_showCardRead = true;

unsigned button_count;
unsigned detect_count;

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


class UrlWatch *watch_cfg = new UrlWatch("acidburn", 80, "/~ben/haccess/cardlist.txt");

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
    while (true) { ESP.wdtFeed(); delay(1);
      Wire.beginTransmission(MCP_IICADDR);
      Wire.write(0x00);
     ret = Wire.endTransmission();
    }
  }
  
  // set pull-ups for various pins
  gpio_exp_wr(MCP_GPPU, 0x3);   // todo - check for correct settings

  // set no input to non inverted
  gpio_exp_wr(MCP_IPOL, 0x0);

  // set output values
  gpio_exp_initout(0x80);       // start with the led off

  // set the io-direction to make GP[7..4] to output
  gpio_exp_wr(MCP_IODIR, byte(0xff ^ 0xF0));

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
  ssid = r;

  r = f.readStringUntil('\n');
  Serial.print("Password ");
  Serial.println(r);
  pass = r;

  f.close();
}

static void start_wifi(void)
{
  WiFi.begin(ssid.c_str(), pass.c_str());
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
  if (! versiondata) {
    Serial.println("Didn't find PN53x board");
    return;
  }

  if ((versiondata >> 24) != 0x32) {
    Serial.println("Device is not PN532?");
  }

  Serial.print("NFC version info is ");
  Serial.println(versiondata, HEX);

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

  pcd8544_settings.resetPin = 32 + 4; // 4;
  //pcd8544_settings.scePin = 16; //5;
  pcd8544_settings.scePin = 32+5;

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
  Serial.printf("topic '%s', payload '%s', lenght %d\n", topic, payload, length);
  // nothing we really care about here at the moment
}

static void setup_mqtt(void)
{
  char servname[128];
  char tmp[32];
  uint16_t port;

  if (cfgfile.getValue("mqtt", "Server", servname, sizeof(servname))) {
    // that's ok
    Serial.println(servname);
  } else {
    Serial.println("default mqtt server");
    sprintf(servname, "acidburn");
  }

  if (cfgfile.getValue("mqtt", "Port", tmp, sizeof(tmp), port)) {
    // ok
    Serial.print("mqtt port ");
    Serial.println(port, DEC);
  } else {
    Serial.println("default mqtt port");
    port = 1883;
  }

  mqtt_server = strdup(servname);
  mqtt.setServer(mqtt_server, port);
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

  cfgfile.setFile(cfg);
}

void setup() {
  Wire.begin(4, 5);
  Serial.begin(115200);

#if defined(PN_IIC) || defined(NFC_ELECHOUSE)
  // ensure the pins for spi are not configured
  pinMode(14, OUTPUT);
  pinMode(12, INPUT);
  pinMode(13, OUTPUT);
  pinMode(16, OUTPUT);
#endif

  // show the chip and sdk version
  show_ids();
  setup_gpioexp();

  gpio_exp_setgpio(6, 1);
  setupDisplay();

  Serial.println("MAC " + WiFi.macAddress());

  fs_opened = SPIFFS.begin();
  if (!fs_opened) {
    Serial.println("Failed to open SPIflash");
  }

  open_config_file();

  read_wifi_config();
  start_wifi();
  setup_mqtt();
  init_nfc();

  check_card_list();


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

static bool show_card = true;

static void checkForCard(void)
{
  boolean success;
  uint8_t uid[8] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;  // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  class CardInfo info;

  //Serial.println("Reading card");
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

    if (lookupCard(&info, uid, uidLength)) {
      Serial.println("Found card " + info.logname);
      show_known_card(&info, uid, uidLength);
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

    PCD8544_gotoXY(64 + 8, 0);
    PCD8544_lcdCharacter((gpio & 1) ? 'x' : '-');
    PCD8544_lcdCharacter((gpio & 2) ? 'x' : '-');
    PCD8544_lcdCharacter((gpio & 4) ? 'x' : '-');
    old_gpio = gpio;

    gpio_exp_setgpio(7, (gpio & 4) ? true : false);

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
const unsigned long cardInterval = 250;
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

#define WDT_ADDR  (0x50)

// pet the external watchdog (if it exists)
static void process_wdt(void)
{
  Wire.beginTransmission(WDT_ADDR);
  Wire.write(0x29);
  Wire.endTransmission();
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

  if (timeTo(&lastCard, cardInterval, curtime) && true)
    checkForCard();

  if (timeTo(&lastFile, fileInterval, curtime))
    checkForNewFiles();

  if (timeTo(&lastTimer, 1000, curtime)) {
    runDisplay();
    //processMqtt();
    sayHello();
    process_wdt();
  }

  // always check gpios through the loop
  checkGPIOs();

  if (Serial.available() > 0) {
    serial_interaction();
  }
}
