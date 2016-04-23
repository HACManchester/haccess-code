
#include <ESP8266WiFi.h>
#include "urlwatch.h"

static const bool debug = false;

enum url_result UrlWatch::upToDate(void)
{
  enum url_result ret = URL_SAME;
  bool got_modified = false;
  WiFiClient client;
  String line;

  // if the wifi is not connected, do not try this
  if (WiFi.status() != WL_CONNECTED) {
    return URL_FETCH_ERR;
  }

  if (!client.connect(this->host, this->port)) {
    return URL_FETCH_ERR;
  }

  client.print("HEAD ");
  client.print(this->url);
  client.print(" HTTP/1.0\r\n");
  client.print("Host: ");
  client.printf(this->host);
  client.print("\r\n\r\n");

  while (client.connected()) {
    if (!client.available())
      continue;

    line = client.readStringUntil('\n');
    if (debug)
      Serial.println("line: " + line);

    if (line.startsWith("Last-Modified:")) {
      String lmdate = line.substring(line.indexOf(" "));
      got_modified = true;

      if (!lmdate.equals(this->last)) {
        this->last = lmdate;
        ret = URL_CHANGED;
      }
    }
  }

  if (!got_modified)
    ret = URL_FETCH_ERR;

  return ret;
}

