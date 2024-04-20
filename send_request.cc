#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAXLINE 1024

int main(void)
{
  int port_number = 80;
  char *host = "warren.sewanee.edu";
  char *message = "GET /index.html HTTP/1.0\r\n"
                  "Host: asd\r\n"
                  "\r\n";

  struct hostent *server = NULL;
  struct sockaddr_in serv_addr = {0};
  int sockfd = 0;
  int bytes = 0;
  int sent = 0;
  int received = 0;
  int total = 0;
  
  char response[1 << 16] = {0};

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server = gethostbyname(host);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port_number);
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "ERROR connecting");
    exit(1);
  }

  total = strlen(message);
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

  total = sizeof(response) - 1;
  received = 0;
  do {
    bytes = read(sockfd, response + received, total - received);
    if (bytes < 0) {
      fprintf(stderr, "ERROR reading response from socket");
      exit(1);
    }
    if (bytes == 0) {
      break;
    }
    received += bytes;
  } while (received < total);

  close(sockfd);

  printf("Response: \n%s\n", response);
}
