#ifndef BLOCKLIST_H
#define BLOCKLIST_H
#define MAXBLOCKSITES 256

struct Blocklist {
  int num_blocked;
  char *sites[MAXBLOCKSITES];
  int sites_lens[MAXBLOCKSITES];
  char text[1<<18];
};

int check_block(struct Blocklist list, char *uri);
#endif
