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
  const char *iphost;
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

  iphost = "85.119.83.178";   // temporary overide.
  if (true)
    Serial.printf("Updating cards from %s:%d url %s\n", host, port, url);

  if (!client.connect(iphost, port)) {
    Serial.print("could not connect to host ");
    Serial.println(host);
    return false;
  }
  
  if (true)
    Serial.printf("issuing get request\n");

  client.print("GET ");
  client.print(url);
  client.printf(" HTTP/1.0\r\nHost: ");
  client.print(host);
  client.print("\r\n");
  //client.print("Accept: */*\r\n");
  //client.print("Accept-Encoding: identity\n");
  client.print("User-Agent: Haccess Node 1.0\r\n");
  if (auth) {
    client.print("Authorization: Basic ");
    client.print(auth);
    client.print("\r\n");
  }
  client.print("\r\n");

  if (true)
    Serial.println("Awaiting response");

  // read the header
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    Serial.printf("CARD HEADER : %s\n", line.c_str());
    //Serial.println(line.length());
    if (line.length() < 2)
      break;
  }

 if (true)
    Serial.println("Reading data");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    Serial.printf("CARD: %s\n", line.c_str());
    f.println(line);
  }

  f.close();
  if (true)
    Serial.println("Read card data from server");
  
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

