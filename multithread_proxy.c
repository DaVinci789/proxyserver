#include "csapp.h"
#include "uriparse.h" // Ask carl if external libraries are allowed.

#define LOGLIST_IMPLEMENTATION
#include "loglist.h"

typedef struct thread_data {
  int connection_fd;
  struct LogList *logger;
} thread_data;

typedef struct log_thread_data {
  FILE *log_file_fd;
  struct LogList *head;
} log_thread_data;

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

void handle_request(int connection_fd, struct LogList *logger)
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

  struct uri uri_parse = {0};
  uriparse(uri, &uri_parse);
  char *host = uri_parse.host;
  char *page = uri_parse.path;
  char *port_string = uri_parse.port;

  char logged_message[32768] = {0};
  int log_len = sprintf(logged_message, "URI: %s Host: %s Page: %s Port: %s\n", uri, host, page, port_string);
  log_message(logger, logged_message, log_len);

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

void *handle_client(void *vargp)
{
  thread_data *thread_data = vargp;
  handle_request(thread_data->connection_fd, thread_data->logger);
  Close(thread_data->connection_fd);
  free(vargp);
  return NULL;
}

void *logging(void *vargp)
{
  log_thread_data *data = (log_thread_data *) vargp;
  struct LogList *head = data->head;
  while (1) {
    struct LogListVisitor visitor = {0};
    visitor.current_list = head;
    int len = 0;
    char *message = NULL;
    int dirty = 0;
    while (get_messages(&visitor, &len, &message)) {
      if (len == 0) continue; // @Hack @ThisIsALaterProblemDontFixThisForThisProjectButInTheFuture
                              // The reason for this check is because a single iteration of advancing
                              // the visitor to the next node is done every loop.
                              // That is, all log message nodes may not have text, but we still need
                              // run the while loop to figure that out.
      dirty = 1;
      printf("%.*s", len, message);
      fprintf(data->log_file_fd, "%.*s", len, message);
    }
    if (dirty) fflush(data->log_file_fd);
  }
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  int listen_fd  = Open_listenfd(argv[1]);

  log_thread_data *log_thread_data = malloc(sizeof(*log_thread_data));

  FILE *logfile = fopen("threadlog.log", "w");
  struct LogList *head = init_loglist();
  struct LogList **tail = &head->next;

  log_thread_data->log_file_fd = logfile;
  log_thread_data->head = head;

  pthread_t logging_id = {0};
  pthread_create(&logging_id, NULL, &logging, (void *) log_thread_data);
  pthread_detach(logging_id);

  while (1) {
    struct sockaddr_storage clientaddr = {0};
    socklen_t client_len = sizeof(clientaddr);
    thread_data *thread_data = malloc(sizeof(*thread_data));
    thread_data->connection_fd = accept(listen_fd, (struct sockaddr *) &clientaddr, &client_len);

    log_message(head, "Accepted new connection!\n", sizeof("Accepted new connection!\n"));

    *tail = init_loglist();
    thread_data->logger = *tail;
    tail = &((*tail)->next); // @Style. Don't do this.

    pthread_t thread_id = {0};
    pthread_create(&thread_id, NULL, &handle_client, (void *) thread_data);
    pthread_detach(thread_id);
  }

  close(listen_fd);
  fclose(logfile);
  destroy_loglist(head);
  return 0;
}
