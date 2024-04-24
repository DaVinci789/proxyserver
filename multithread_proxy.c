#include "csapp.h"

#include "requests.h"

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
