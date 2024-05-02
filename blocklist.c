/* blocklist.c - List data structure to return if a uri is in itself.
 * TEAM MEMBERS:
 *      Jai Manacsa
 *      Debbie Lim
 *      Utsav Bhandari
 */

#include "blocklist.h"

#include <string.h>
#include <stdio.h>

#define MAXLINE 8192
#define pvar(s) printf(#s": %s\n", s)

<<<<<<< HEAD
static void *memmem(void *haystack, size_t haystacklen, void *needle, size_t needlelen);

int check_block(struct Blocklist list, char *uri)
=======
// Check if uri is on the blocklist
int check_block(struct Blocklist list, char *host, char *page)
>>>>>>> refs/remotes/origin/main
{
  for (int i = 0; i < list.num_blocked; i++) {
    #define STATIC(s) s, sizeof(s) - 1
    char *uri_sans_protocol = uri;
    char *has_http = memmem(uri, strlen(uri), STATIC("http"));
    if (has_http) {
      uri_sans_protocol = has_http;
      uri_sans_protocol += sizeof("http:") - 1;
      if (*uri_sans_protocol == 's') {
        uri_sans_protocol += 1;
      }
      uri_sans_protocol += sizeof("//") - 1;
    }
    uri = uri_sans_protocol;
    #undef STATIC

    char *asterisk = memmem(list.sites[i], list.sites_lens[i], "*", 1);
    if (asterisk) {
      if (0 == strncmp(list.sites[i], uri, asterisk - list.sites[i] - 1)) {
        return 0;
      }
    } else {
      if (strlen(uri) - 1 != list.sites_lens[i]) continue;
      if (0 == strncmp(list.sites[i], uri, strlen(uri) - 1)) {
        return 0;
      }
    }
  }
  return 1;
}

static void *memmem(void *haystack, size_t haystacklen, void *needle, size_t needlelen)
{
  char const *bf = haystack;
  char const *pt = needle;
  char *p = haystack;
 
  while (needlelen <= (haystacklen - (p - bf))) {
    if (NULL != (p = memchr(p, (int)(*pt), haystacklen - (p - bf)))) {

      // checks if the 'size of p,' that is, the difference between p and the end of the buffer
      // is smaller than the length of the needle.
      // (I wonder why this check is necessary... I assumed the `while` would have taken care of it)
      if (needlelen > ((bf + haystacklen) - p)) break; 

      if (0 == memcmp(p, needle, needlelen)) {
        return p;
      } else {
        p += 1;
      }
    } else {
      break;
    }
  }
 
  return NULL;
}
