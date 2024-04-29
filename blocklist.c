#include "blocklist.h"
#include <string.h>
#include <stdio.h>

#define MAXLINE 8192

int check_block(struct Blocklist list, char *host, char *page)
{
  for (int i = 0; i < MAXBLOCKSITES; i++) {
    if (!list.sites_lens[i]) continue;
    char uri[MAXLINE] = {0};
    sprintf(uri, "%s/%s", host, page);
    if (0 == strncmp(list.sites[i], uri, list.sites_lens[i]))
      return 0;
  }
  return 1;
}
