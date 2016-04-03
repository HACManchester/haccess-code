// Haccess v1 Access Control System
// Copyright 2015 Ben Dooks <ben@fluff.org>

class CardInfo {
  public:
    CardInfo() { };
    ~CardInfo() { };
    String shortname;
    String logname;
};

extern bool dbg_showCardRead;

extern void readCardList(File f);
extern bool copyCardList(const char *host, unsigned port, const char *url);

extern bool lookupCard(class CardInfo *info, uint8_t *uid, int uidLength);


