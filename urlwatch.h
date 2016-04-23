// Haccess v1 Access Control System
// Copyright 2015 Ben Dooks <ben@fluff.org>

enum url_result {
  URL_FETCH_ERR,
  URL_CHANGED,
  URL_SAME
};

class UrlWatch {
 public:
  UrlWatch(const char *host, unsigned int port, const char *url) {
    this->host = host;
    this->port = port;
    this->url = url;
  }

  enum url_result upToDate(void);

  const char *getHost() { return this->host; }
  const char *getUrl() { return this->url; }
  const unsigned int getPort() { return this->port; };
  
 private:
  String last;			// last modification date from host
  
  const char *host;
  const char *url;
  unsigned int port;

};

