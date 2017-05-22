
#include "FS.h"
#include "IniFile.h"

#include "haccess.h"
#include "config.h"


// configuration file
File cfg_file;
IniFile cfgfile;

// main configuration info
struct config cfg = {
  .en_rfid = true,
  .en_mqtt = false,
  .en_cards = false,
  .en_cards_watch = false,
  .en_cards_fetch = false,
  .en_cards_update = false,
  .rfid_interval = 250,
  .wifi_ssid = "Hackspace",
  .wifi_pass = "T3h4x0rZ",
  .card_interval = 600,
  .card_auth = NULL,
  .card_host = NULL,
  .card_url = NULL,
  .mqtt_server = NULL,
  .mqtt_port = 1883,
};
