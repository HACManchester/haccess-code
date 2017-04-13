// Haccess v1 Access Control System
// Copyright 2015 Ben Dooks <ben@fluff.org>

extern bool fs_opened;

struct config {
  bool en_rfid;     /**< Set if RFID read is required */
  bool en_mqtt;     /**< Set to use MQTT */

  bool en_cards;
  bool en_cards_fetch;
  bool en_cards_update;

  unsigned int rfid_interval;   /**< RFID poll interval */

  const char *wifi_ssid;
  const char *wifi_pass;

  const char *card_host;
  const char *card_url;

  const char *mqtt_server;
  uint16_t mqtt_port;
};

extern IniFile cfgfile;
extern File cfg_file;
