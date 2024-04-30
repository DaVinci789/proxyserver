#include "csapp.h"

#include "requests.h"
#include "blocklist.h"

// Sequential logging - log messages written in the order they are generated
#define LOGLIST_SEQUENTIAL
FILE *LOGLIST_SEQUENTIAL_FD = NULL;
#define LOGLIST_IMPLEMENTATION

// Logging for debugging purposes
#ifndef RELEASE
  #define LOGLIST_PRINTF
#endif

#include "loglist.h"

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  int listen_fd  = Open_listenfd(argv[1]);

  FILE *logfile = fopen("proxy.log", "w");
  LOGLIST_SEQUENTIAL_FD = logfile;
  struct LogList *head = init_loglist();

  // Initialize blocklist
  struct Blocklist list = {0}; // @TODO. File parsing
/*   list.sites[0] = "warren.sewanee.edu/";
  list.sites_lens[0] = sizeof("warren.sewanee.edu/") - 1; */

  // Hard coded blocked site?
  list.sites[0] = "httpforever.com/";
  list.sites_lens[0] = sizeof("httpforever.com/") - 1;

  while (1) {
    struct sockaddr_in clientaddr = {0};
    socklen_t client_len = sizeof(clientaddr);
    int connection_fd = accept(listen_fd, (struct sockaddr *) &clientaddr, &client_len);
    
    // Convert IP address (in network byte order) into readable string 
    char *client_ip = inet_ntoa(clientaddr.sin_addr);
    handle_request(connection_fd, client_ip, list, head);
    Close(connection_fd);
  }

  close(listen_fd);
  fclose(LOGLIST_SEQUENTIAL_FD);
  return 0;
}

