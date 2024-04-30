#include "requests.h"
#include "validate_uri.h"
#include "blocklist.h"
#include "csapp.h"
#include "uriparse.h" // Ask carl if external libraries are allowed.
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

// Find substring in the string
static void *memmem(void *haystack, size_t haystacklen, void *needle, size_t needlelen)
{
  char const *bf = haystack;
  char const *pt = needle;
  char *p = haystack;
 
  while (needlelen <= (haystacklen - (p - bf))) {
    // Search for first character of needle, if found, p is updated to point to this location
    if (NULL != (p = memchr(p, (int)(*pt), haystacklen - (p - bf)))) {

      // checks if the 'size of p,' that is, the difference between p and the end of the buffer
      // is smaller than the length of the needle.
      // (I wonder why this check is necessary... I assumed the `while` would have taken care of it)
      
      // Check there is enough space in haystack to fit entire needle
      // If not then break as further search is unnecessary (this is for efficiency)
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

// ci stands for case insensitive
// Why do we have 2 separate functions for this, could they not be put in one?
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
  char *result = memmem(haystack_lower, haystacklen, needle_lower, needlelen);
  free(haystack_lower);
  free(needle_lower);
  if (!result) return result;
  return ((char*) haystack) + (result - haystack_lower);
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

  // Create socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  // Get host info
  server = gethostbyname(host);
  if (!server) {
    *response_out_len = 0;
    return NULL;
  }

  // Initialize server address structure
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port_number);
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

  // Connect to server
  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "ERROR connecting\n");
    return NULL;
  }

  // Send HTTP request message
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

  // Send HTTP response message
  // Why 2^13? Well, the http spec doesn't specify a maximum limit
  // on the number of headers a request can send, but we need to
  // allocate a buffer of *some* size, still!
  // Apparently, most web servers allocate an 8K buffer upfront[1]
  // and going over that limit results in a 413 Request too large
  // error, so we're following in that tradition.
  // [1] https://stackoverflow.com/questions/686217/maximum-on-http-header-values
  char *response = calloc(1 << 13, 1);
  int resized_buffer = 0;
  total = 1 << 13;
  received = 0;
  do {
    bytes = read(sockfd, response + received, total - received);
    if (bytes < 0) {
      fprintf(stderr, "ERROR reading response from socket\n");
      return NULL;
    }

    received += bytes;

    // Resize buffer if content-length header found
    if (resized_buffer == 0) {
      char *content_length_location = memmem_ci(response, received,
                                             "Content-Length: ", sizeof("Content-Length: ") - 1);
      if (content_length_location != NULL) {
        resized_buffer = 1;

        char content_length_str[MAXLINE] = {0};
        sscanf(content_length_location, "%*[Cc]ontent-%*[Ll]ength: %99[^\r]\r\n", content_length_str);
        int content_length = atoi(content_length_str);

        if (content_length > total) {
          total += content_length;
          response = realloc(response, total);
        }
      }
    }
  } while (bytes != 0);

  // Signal end of response
  char the_eof = EOF;
  write(sockfd, &the_eof, 1);
  close(sockfd);
  *response_out_len = received;
  return response;
}

void handle_request(int connection_fd, char *client_ip, struct Blocklist blocklist, struct LogList *logger)
{
  rio_t rio = {0};
  char method[MAXLINE] = {0};
  char uri[MAXLINE] = {0};
  char version[MAXLINE] = {0};

  char *buf = malloc(MAXLINE);
  char *current = buf;
  int buf_size = MAXLINE;
  rio_readinitb(&rio, connection_fd);
  ptrdiff_t http_content_size = 0;
  ptrdiff_t http_content_total = 0;

  http_content_total = rio_readlineb(&rio, current, MAXLINE);

  // read the http request.
  // First line of the request is the http method, uri, version
  sscanf(current, "%s %s %s", method, uri, version);

  // Sanitize uri for malformed addresses/malicious input
  char *cleaned_uri = sanitize_uri(uri);
  if (!cleaned_uri) return;
  printf("%s \n", cleaned_uri);
  memcpy(uri, cleaned_uri, strlen(cleaned_uri) + 1);
  free(cleaned_uri);
  cleaned_uri = NULL;

  struct uri uri_parse = {0};
  uriparse(uri, &uri_parse);
  char *host = uri_parse.host;
  if (!host) host = "NOHOST";

  char *page = uri_parse.path;
  if (!page) page = "/";
  page += 1; // @Safety. This can mess up with the literal above.
  char *port_string = uri_parse.port;

  // Check if uri is on the blocklist
  // Shouldn't this be before sanitizing?
  if (!check_block(blocklist, host, page)) {
    char blockmessage[MAXLINE] = {0};
    int len = sprintf(blockmessage, "Alert! Client attempted to access %s!\n", host);
    log_message(logger, blockmessage, len);
    return;
  }

  // Make HTTP GET request message
  http_content_total = sprintf(current, "GET /%s HTTP/1.0\r\n", page);
  buf_size += MAXLINE;

  // Read remaining headers and modify if needed
  do {
    buf = realloc(buf, buf_size);
    current = buf + http_content_total;
    http_content_size = rio_readlineb(&rio, current, MAXLINE);

    #define STATIC(s) s, sizeof(s) - 1
    if (memmem_ci(current, http_content_size, STATIC("User-Agent"))) {
      continue;
    }

    if (memmem_ci(current, http_content_size, STATIC("Connection: keep-alive"))) {
      strcpy(current, "Connection: close\r\n");
      http_content_size = sizeof("Connection: close\r\n") - 1;
    }
    #undef STATIC

    http_content_total += http_content_size;
    buf_size += MAXLINE;
  } while (NULL == memmem("\r\n", sizeof("\r\n") - 1, current, http_content_size));

  char logged_message[32768] = {0};
  int log_len = sprintf(logged_message, "%s %s: %s %ld\n", client_ip , host, page, http_content_total);
  log_message(logger, logged_message, log_len);

  // Default port number is 80
  int port = 80;
  if (port_string) {
    port = atoi(port_string);
  }

  int response_len = 0;
  char *response = send_request(port, host, buf, http_content_total, &response_len);
  Write(connection_fd, response, response_len);
  free(response);
  free(buf);
}
