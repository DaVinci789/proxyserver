#include "csapp.h"

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
    fprintf(stderr, "ERROR connecting");
    exit(1);
  }

  total = message_strlen;
  sent = 0;
  do {
    bytes = write(sockfd, message + sent, total - sent);
    if (bytes < 0) {
      fprintf(stderr, "ERROR writing message to socket");
      exit(1);
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
      fprintf(stderr, "ERROR reading response from socket");
      exit(1);
    }

    received += bytes;

    char *content_length_location = memmem(response, received, "Content-Length: ", sizeof("Content-Length: ") - 1);
    if (resized_buffer == 0 &&
	content_length_location != NULL) {
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
  } while (bytes != 0);//while (received < total);

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

  char host[MAXLINE] = {0}; 
  char page[MAXLINE] = {0};
  sscanf(uri, "http://%99[^/]/%99[^\n]", host, page);
  printf("%s %s\n", host, page);

  // big number here cause gcc tells me that host (of size MAXLINE) could be
  // too chonky for a `message` var of size MAXLINE.
  // so make it large enough that it can fit a `page` the size of MAXLINE.
  char message[32768] = {0}; // 32768 == 2 * 2 * 8192 (MAXLINE constant).
                             // IDK if doing MAXLINE * 2 makes this a VLA.
  sprintf(message, "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", page, host);

  int response_len = 0;
  char *response = send_request(80, host, message, strlen(message), &response_len);
  if (!response) return;

  Write(connection_fd, response, response_len);
  free(response);
  response = NULL;
}

void *handle_client(void *vargp)
{
  int connection_fd = *(int*) vargp;
  free((int*) vargp);

  handle_request(connection_fd);
  Close(connection_fd);
  return NULL;
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
    int *connection_fd = malloc(sizeof(*connection_fd));
    *connection_fd = accept(listen_fd, (struct sockaddr *) &clientaddr, &client_len);
    printf("Accepted new connection!\n");
    pthread_t thread_id = {0};
    pthread_create(&thread_id, NULL, &handle_client, (void *) connection_fd);
    pthread_detach(thread_id);
  }

  close(listen_fd);
  return 0;
}
