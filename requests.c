#include "requests.h"
#include "validate_uri.h"
#include "csapp.h"
#include "uriparse.h" // Ask carl if external libraries are allowed.
#include <time.h>
#include <stdlib.h>
#include <string.h>

static void *memmem(void *haystack, size_t haystacklen, void *needle, size_t needlelen)
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

// ci stands for case insensitive
static void *memmem_ci(void *haystack, size_t haystacklen, void *needle, size_t needlelen)
{
  char *haystack_lower = malloc(haystacklen);
  char *needle_lower = malloc(needlelen);

  size_t haystacklen_copy = haystacklen;
  while (haystacklen_copy) {
    haystack_lower[haystacklen_copy - 1] = tolower(((char*)haystack)[haystacklen_copy - 1]);
    haystacklen_copy -= 1;
  }
  size_t needlelen_copy = needlelen;
  while (needlelen_copy) {
    needle_lower[needlelen_copy - 1] = tolower(((char*)needle)[needlelen_copy - 1]);
    needlelen_copy -= 1;
  }

  return memmem(haystack_lower, haystacklen, needle_lower, needlelen);
}

char *send_request(int port_number, char *host, char *message, int message_strlen, int *response_out_len)
{
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
      char *content_length_location = memmem_ci(response, received,
                                             "Content-Length: ", sizeof("Content-Length: ") - 1);
      if (content_length_location != NULL) {
        resized_buffer = 1;

        char content_length_str[MAXLINE] = {0};
        sscanf(content_length_location, "%*[Cc]ontent-%*[Ll]ength: %99[^\r]\r\n", content_length_str);
        int content_length = atoi(content_length_str);
        printf("%d\n", content_length);

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

void handle_request(int connection_fd, char *client_ip, struct LogList *logger)
{
  rio_t rio = {0};
  char buf[MAXLINE] = {0};
  char method[MAXLINE] = {0};
  char uri[MAXLINE] = {0};
  char version[MAXLINE] = {0};

  rio_readinitb(&rio, connection_fd);
  if (!rio_readlineb(&rio, buf, MAXLINE)) {
    return;
  }

  // read the http request
  sscanf(buf, "%s %s %s", method, uri, version);
  char *cleaned_uri = sanitize_uri(uri);
  if (!cleaned_uri) return;
  memcpy(uri, cleaned_uri, strlen(cleaned_uri) + 1);
  free(cleaned_uri);
  cleaned_uri = NULL;

  struct uri uri_parse = {0};
  uriparse(uri, &uri_parse);
  char *host = uri_parse.host;
  if (!host) host = "NOHOST";

  char *page = uri_parse.path;
  if (!page) page = "/";
  page += 1;
  char *port_string = uri_parse.port;

  // Get the date and time
  time_t timeRN = {0};
  struct tm *timeinfo = NULL;
  char timestamp[8000] = {0};        // proxy will close after this amount of time
  time(&timeRN);
  timeinfo = localtime(&timeRN);
  strftime(timestamp, sizeof(timestamp), "%m-%d-%Y %H:%M:%S", timeinfo);

  int uri_size = 0;

  char logged_message[32768] = {0};
  /*int log_len = sprintf(logged_message, "[%s] %s %s: %s %d\n", timestamp, client_ip , host, page, uri_size);
  log_message(logger, logged_message, log_len);*/
  printf("[%s] %s %s: %s %d\n", timestamp, client_ip , host, page, uri_size);

  // big number here cause gcc tells me that host (of size MAXLINE) could be
  // too chonky for a `message` var of size MAXLINE.
  // so make it large enough that it can fit a `page` the size of MAXLINE.
  char message[32768] = {0}; // 32768 == 2 * 2 * 8192 (MAXLINE constant).
                             // IDK if doing MAXLINE * 2 makes this a VLA.
  sprintf(message, "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", page, host);

  int port = 80;
  if (port_string) {
    port = atoi(port_string);
  }

  int response_len = 0;
  char *response = send_request(port, host, message, strlen(message), &response_len);
  Write(connection_fd, response, response_len);
  free(response);
  response = NULL;
}
