#ifndef REQUESTS_H
#define REQUESTS_H

#include "loglist.h"
#include "blocklist.h"

char *send_request(int port_number, char *host, char *message, int message_strlen, int *response_out_len);
void handle_request(int connection_fd, char *client_ip, struct Blocklist list, struct LogList *logger);
#endif