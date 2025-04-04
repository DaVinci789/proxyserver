/* multithread_proxy.c - Runner for the multithreaded/concurrent proxy
 * TEAM MEMBERS:
 *      Jai Manacsa
 *      Debbie Lim
 *      Utsav Bhandari
 */

#include "csapp.h"

#include "requests.h"

#ifndef RELEASE
  #define LOGLIST_PRINTF
#endif

#define LOGLIST_IMPLEMENTATION
#include "loglist.h"

#include "blocklist.h"

typedef struct thread_data {
  int connection_fd;
  char client_ip[256];
  struct Blocklist list;
  struct LogList *logger;
} thread_data;

typedef struct log_thread_data {
  FILE *log_file_fd;
  struct LogList *head;
} log_thread_data;

volatile int exit_status = 0;

void exit_signal(int signum)
{
  exit_status = 1;
}

void *handle_client(void *vargp)
{
  thread_data *thread_data = vargp;
  handle_request(thread_data->connection_fd, thread_data->client_ip, thread_data->list, thread_data->logger); // @TODO Blocklist implimentation
  Close(thread_data->connection_fd);
  free(vargp);
  return NULL;
}

// Function to log client requests, necessary to track activity of the proxy server
void *logging(void *vargp)
{
  log_thread_data *data = (log_thread_data *) vargp;
  struct LogList *head = data->head;
  while (!exit_status) {
    struct LogListVisitor visitor = {0};
    visitor.current_list = head;
    int len = 0;
    char *message = NULL;
    int dirty = 0;
    while (get_messages(&visitor, &len, &message)) {
      dirty = 1;
      fprintf(data->log_file_fd, "%.*s", len, message);
    }
    if (dirty) fflush(data->log_file_fd);
  }
  return NULL;
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // Initialize signal handler
  struct sigaction handler = {0};
  handler.sa_handler = exit_signal;

  // Register SIGINT signal handler
  sigaction(SIGINT, &handler, NULL);

  // Ignore SIGPIPE signal
  sigaction(SIGPIPE, &(struct sigaction){.sa_handler = SIG_IGN}, NULL);

  int listen_fd  = Open_listenfd(argv[1]);

  log_thread_data *log_thread_data = calloc(sizeof(*log_thread_data), 1);

  FILE *logfile = fopen("proxy.log", "w");
  struct LogList *head = init_loglist();
  struct LogList **tail = &head->next;

  log_thread_data->log_file_fd = logfile;
  log_thread_data->head = head;

  // Asynchronously run logging in a separate thread, allows main operaton to continue 
  // Without waiting for logging to complete
  pthread_t logging_id = {0};
  pthread_create(&logging_id, NULL, &logging, (void *) log_thread_data);
  pthread_detach(logging_id);

  int blockfile = open("blocklist.txt", O_RDONLY);
  struct Blocklist list = {0};
  if (blockfile) {
    char *current = list.text;

    rio_t rio = {0};
    rio_readinitb(&rio, blockfile);

    int len = 0;
    while ((len = rio_readlineb(&rio, current, MAXLINE))) {
      list.sites[list.num_blocked] = current;
      list.sites_lens[list.num_blocked] = len - 1;
      list.num_blocked += 1;
      current += len;
    }
  }

  while (!exit_status) {
    struct sockaddr_in clientaddr = {0};
    socklen_t client_len = sizeof(clientaddr);
    thread_data *thread_data = malloc(sizeof(*thread_data));
    thread_data->connection_fd = accept(listen_fd, (struct sockaddr *) &clientaddr, &client_len);
    memcpy(&thread_data->list, &list, sizeof(list));
    strcpy(thread_data->client_ip, inet_ntoa(clientaddr.sin_addr));

    if (exit_status) {
      free(thread_data);
      break;
    }

    char message[MAXLINE] = {0};
    int message_len = sprintf(message, "Accepted connection from %s\n", thread_data->client_ip);
    log_message(head, message, message_len);

    *tail = init_loglist();
    thread_data->logger = *tail;
    tail = &((*tail)->next); // @Style. Don't do this.

    // Separate thread to handle client
    pthread_t thread_id = {0};
    pthread_create(&thread_id, NULL, &handle_client, (void *) thread_data);
    pthread_detach(thread_id);
  }

  free(log_thread_data);
  close(listen_fd);
  fclose(logfile);
  close(blockfile);
  destroy_loglist(head);
  pthread_kill(logging_id, SIGINT);
  return 0;
}
