#include "csapp.h"
#include "uriparse.h" // Ask carl if external libraries are allowed.

void *memmem(void *haystack, size_t haystacklen, void *needle, size_t needlelen)
{
  char *bf = haystack;
  char *pt = needle;
  char *p = bf;
 
  while (needlelen <= (haystacklen - (p - bf))) {
    if (NULL != (p = memchr(p, (int)(*pt), haystacklen - (p - bf)))) {
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

char *send_request(int port_number, char *host, char *message, int message_strlen, int *response_out_len) {
  struct hostent *server = NULL;
  struct sockaddr_in serv_addr = {0};
  int sockfd = 0;
  int bytes = 0;
  int sent = 0;
  int received = 0;
  long total = 0;
  *response_out_len = 0;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server = gethostbyname(host);
  if (!server) {
    *response_out_len = 0;
    return NULL;
  }
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port_number);
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "ERROR connecting\n");
    return NULL;
  }

  total = message_strlen;
  sent = 0;
  do {
    bytes = write(sockfd, message + sent, total - sent);
    if (bytes < 0) {
      fprintf(stderr, "ERROR writing message to socket\n");
      return NULL;
      //exit(1);
    }
    if (bytes == 0) {
      break;
    }
    sent += bytes;
  } while (sent < total);

  // Why 2^13? Well, the http spec doesn't specify a maximum limit
  // on the number of headers a request can send, but we need to
  // allocate a buffer of *some* size, still!
  // Apparently, most web servers allocate an 8K buffer upfront[1]
  // and going over that limit results in a 413 Request too large
  // error, so we're following in that tradition.
  // [1] -- https://stackoverflow.com/questions/686217/maximum-on-http-header-values
  char *response = calloc(1 << 13, 1);
  int resized_buffer = 0;
  total = 1 << 13;
  received = 0;
  do {
    bytes = read(sockfd, response + received, total - received);
    if (bytes < 0) {
      fprintf(stderr, "ERROR reading response from socket\n");
      return NULL;
      //exit(1);
    }

    received += bytes;

    if (resized_buffer == 0) {
      char *content_length_location = memmem(response, received,
                                             "Content-Length: ", sizeof("Content-Length: ") - 1);
      if (content_length_location != NULL) {
        resized_buffer = 1;

        char content_length_str[MAXLINE] = {0};
        sscanf(content_length_location, "Content-Length: %99[^\r]\r\n", content_length_str);
        int content_length = atoi(content_length_str);

        if (content_length > total) {
          printf("Allocate!\n");
          total += content_length;
          response = realloc(response, total);
        }
      }
    }
  } while (bytes != 0);

  close(sockfd);
  *response_out_len = received;
  return response;
}

void handle_request(int connection_fd)
{
  rio_t rio = {0};
  char buf[MAXLINE] = {0};
  char method[MAXLINE] = {0};
  char uri[MAXLINE] = {0};
  char version[MAXLINE] = {0};

  Rio_readinitb(&rio, connection_fd);
  if (!Rio_readlineb(&rio, buf, MAXLINE)) {
    return;
  }

  // read the http request
  sscanf(buf, "%s %s %s", method, uri, version);

  /*char host[MAXLINE] = {0}; 
  char page[MAXLINE] = {0};
  sscanf(uri, "http://%99[^/]/%99[^\n]", host, page);*/

  struct uri uri_parse = {0};
  uriparse(uri, &uri_parse);
  char *host = uri_parse.host;
  char *page = uri_parse.path;
  char *port_string = uri_parse.port;

  printf("URI: %s Host: %s Page: %s Port: %s\n", uri, host, page, port_string);

  // big number here cause gcc tells me that host (of size MAXLINE) could be
  // too chonky for a `message` var of size MAXLINE.
  // so make it large enough that it can fit a `page` the size of MAXLINE.
  char message[32768] = {0}; // 32768 == 2 * 2 * 8192 (MAXLINE constant).
                             // IDK if doing MAXLINE * 2 makes this a VLA.
  sprintf(message, "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", page, host);

  int port = 80;
  if (port_string)
    port = atoi(port_string);

  int response_len = 0;
  char *response = send_request(port, host, message, strlen(message), &response_len);
  Write(connection_fd, response, response_len);
  free(response);
  response = NULL;
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  int listen_fd  = Open_listenfd(argv[1]);

  // pthread_create(logging thread);

  while (1) {
    struct sockaddr_storage clientaddr = {0};
    socklen_t client_len = sizeof(clientaddr);
    int connection_fd = accept(listen_fd, (struct sockaddr *) &clientaddr, &client_len);
    handle_request(connection_fd);
    Close(connection_fd);
  }

  close(listen_fd);
  return 0;
}
