#include "loglist.h"

char *send_request(int port_number, char *host, char *message, int message_strlen, int *response_out_len);
void handle_request(int connection_fd, struct LogList *logger);