#include "csapp.h"

// #define LOGLIST_IMPLEMENTATION
// #include "loglist.h"

#include <stdio.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
// static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int parse_uri(char *uri, char *filename_out, char *cgiargs_out) 
{
  char *ptr;

  if (!strstr(uri, "cgi-bin")) {  /* Static content */ //line:netp:parseuri:isstatic
    strcpy(cgiargs_out, "");                             //line:netp:parseuri:clearcgi
    strcpy(filename_out, ".");                           //line:netp:parseuri:beginconvert1
    strcat(filename_out, uri);                           //line:netp:parseuri:endconvert1
    if (uri[strlen(uri)-1] == '/')                   //line:netp:parseuri:slashcheck
      strcat(filename_out, "home.html");               //line:netp:parseuri:appenddefault
    return 1;
  }
  else {  /* Dynamic content */                        //line:netp:parseuri:isdynamic
    ptr = index(uri, '?');                           //line:netp:parseuri:beginextract
    if (ptr) {
      strcpy(cgiargs_out, ptr+1);
      *ptr = '\0';
    }
    else {
      strcpy(cgiargs_out, "");                         //line:netp:parseuri:endextract
    }
    strcpy(filename_out, ".");                           //line:netp:parseuri:beginconvert2
    strcat(filename_out, uri);                           //line:netp:parseuri:endconvert2
    return 0;
  }
}

void handle_request(int connfd)
{
  rio_t rio = {0};
  char buf[MAXLINE] = {0};
  char method[MAXLINE] = {0};
  char uri[MAXLINE] = {0};
  char version[MAXLINE] = {0};

  Rio_readinitb(&rio, connfd);
  if (!Rio_readlineb(&rio, buf, MAXLINE)) {
    return;
  }
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  {
    char header_buffer[MAXLINE] = {0};
    Rio_readlineb(&rio, header_buffer, MAXLINE);
    printf("%s", header_buffer);
    while(strcmp(header_buffer, "\r\n")) {          //line:netp:readhdrs:checkterm
      Rio_readlineb(&rio, header_buffer, MAXLINE);
      printf("%s", header_buffer);
    }
  }

  char filename[MAXLINE] = {0};
  char cgiargs[MAXLINE] = {0};
  int is_static = parse_uri(uri, filename, cgiargs);
  printf("Data stuff: z:%d a:%s b:%s c:%s\n", is_static, uri, filename, cgiargs);
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  char hostname[MAXLINE] = {0};
  char port[MAXLINE] = {0};
  int listenfd = Open_listenfd(argv[1]); 
  while (1) {
    struct sockaddr_storage clientaddr = {0};
    socklen_t clientlen = sizeof(clientaddr);
    int connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    handle_request(connfd);
    Close(connfd);
  }
  return 0;
}
