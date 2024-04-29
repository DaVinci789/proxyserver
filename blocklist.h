#ifndef BLOCKLIST_H
#define BLOCKLIST_H
#define MAXBLOCKSITES 256

struct Blocklist {
  char *sites[MAXBLOCKSITES];
  int sites_lens[MAXBLOCKSITES];
};

int check_block(struct Blocklist list, char *host, char *page);
#endif