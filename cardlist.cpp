// Haccess v1 Access Control System
// Copyright 2015 Ben Dooks <ben@fluff.org>

#include <ESP8266WiFi.h>
#include "IniFile.h"
#include "FS.h"
#include "haccess.h"
#include "cards.h"

static String last_modified;

bool copyCardList(const char *host, unsigned port, const char *url, const char *auth)
{
  WiFiClient client;
   
  File f;

  if (!fs_opened) {
    Serial.println("filesystem not initialised, failing to update card list");
    return false;
  }

  SPIFFS.remove("/cardslist");
  
  f = SPIFFS.open("/cardlist", "w");
  if (!f) {
    Serial.println("failed to open card file to write");
    return false;
  }

  if (!client.connect(host, port)) {
    Serial.print("could not connect to host ");
    Serial.println(host);
    return false;
  }

  //Serial.println("Updating cards from " + host + " " + url);
  
  client.print("GET ");
  client.print(url);
  client.printf(" HTTP/1.0\r\nHost: ");
  client.print(host);
  client.print("\r\n");
  if (auth) {
    client.print("Authorization: Basic ");
    client.print(auth);
    client.print("\r\n");
  }
  client.print("\r\n");

  // read the header
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    Serial.print("CARD HEADER :" + line);
    Serial.println(line.length());
    if (line.length() < 2)
      break;
  }

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    Serial.println("CARD: " + line);
    f.println(line);
  }

  f.close();
  
  if (1) {
    File r = SPIFFS.open("/cardlist", "r");

    if (r) 
    readCardList(r);
    else
    Serial.println("failed to open card file to read");
   }

  
  // assume client gets destroyed here
  return true;  
}

void readCardList(File f)
{
  String l;
  
  while (f.available()) {
    l = f.readStringUntil('\n');
    if (dbg_showCardRead) {
      Serial.print("readCard: line: ");
      Serial.println(l);
    }
    ESP.wdtFeed();
  }
}

bool lookupCard(class CardInfo *info, uint8_t *uid, int uidLength)
{
  char  uid_full[64];
  char *uid_short;
  String uidstr;
  String l;
  unsigned ptr;

  File f = SPIFFS.open("/cardlist", "r");
  if (!f) {
    Serial.println("failed to open card list");
    return false;
  }

  for (ptr = 0; ptr < uidLength; ptr++) {
    sprintf(uid_full + (ptr * 2), "%02x", uid[ptr]);
  }

  Serial.printf("UID: %s\n", uid_full);

  if (uidLength == 4) {
    uid_short = uid_full;
  } else {
    // to fix
    uid_short = uid_full;
  }

  while (f.available()) {
    l = f.readStringUntil('\n');
    if (dbg_showCardRead) {
      Serial.print("checkCard: line: ");
      Serial.println(l);
    }

    int sep = l.indexOf(',', 0);
    if (sep <= 2)
      continue;

#if 0
    Serial.println("read uid is " + l.substring(0,sep));
    uidstr = l.substring(0, sep);
    
    if (uidstr.equalsIgnoreCase(uid_full) || uidstr.equalsIgnoreCase(uid_short)) {
      int nick = l.indexOf(',', sep+1);
      int full = l.indexOf(',', nick+1);

      if (nick == -1 || full == -1)
        continue;

      info->shortname = l.substring(sep+1, nick);
      info->logname = l.substring(nick+1, full);

      return true;
    }
#else
    {
       // format is logname, shortname, uid
       int full = l.indexOf(',', sep+1);
       int ends = l.length();

       if (full == -1)
         continue;
  
       uidstr = l.substring(full+2, ends-1);
       Serial.println("read uid is " + uidstr); 
       if (uidstr.equalsIgnoreCase(uid_full) || uidstr.equalsIgnoreCase(uid_short) || uidstr.startsWith(uid_full)) {

        info->shortname = l.substring(sep+2, full+2);
        info->logname = l.substring(1, sep-1);
        return true;          
      }
    }
#endif
    
  }

  return false;
}

