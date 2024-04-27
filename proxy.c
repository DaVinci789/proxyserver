#include "csapp.h"

#include "requests.h"

#define LOGLIST_SEQUENTIAL
FILE *LOGLIST_SEQUENTIAL_FD = NULL;
#define LOGLIST_IMPLEMENTATION
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


  while (1) {
    struct sockaddr_storage clientaddr = {0};
    socklen_t client_len = sizeof(clientaddr);
    int connection_fd = accept(listen_fd, (struct sockaddr *) &clientaddr, &client_len);
    log_message(head, "Accepted new connection!\n", sizeof("Accepted new connection!\n"));
    handle_request(connection_fd, head);
    Close(connection_fd);
  }

  close(listen_fd);
  fclose(LOGLIST_SEQUENTIAL_FD);
  return 0;
}

  // Getting the browser IP

  // struct sockadd_in client_addr;
  // socklen_t client_len = sizeof(client_addr);
  // if(getpeername(connection_fd, (struct sockaddr *) &client_addr, &client_len) == 0)  {
  //   char *browser_ip = inet_ntoa(client_addr.sin_addr);
  // }

