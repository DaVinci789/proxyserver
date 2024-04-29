#ifndef LOGLIST_H
#define LOGLIST_H
#include <pthread.h>
#include <time.h>

#ifndef LOGLIST_MAXBUF
  #define LOGLIST_MAXBUF 1 << 8
#endif

struct LogNode {
  int used_chars;
  char info[LOGLIST_MAXBUF];
  struct LogNode *next;
};

struct LogList {
  pthread_mutex_t lock;
  struct LogList *next;
  int dirty; // Indicates a message has been written to this LogList
  struct LogNode head;
  struct LogNode *tail;
};

struct LogListVisitor {
  struct LogList *current_list;
  struct LogNode *current_node;
};

void log_message(struct LogList *logger, char *message, int message_len);
struct LogList* init_loglist();
void init_loglist2(struct LogList *init);
int get_messages(struct LogListVisitor *visitor, int *len, char **message);
void destroy_loglist(struct LogList *head);
#endif

#ifdef LOGLIST_IMPLEMENTATION
#include <stdlib.h>

void log_message(struct LogList *logger, char *message, int message_len)
{
  // Get the date and time
  time_t timeRN = {0};
  struct tm *timeinfo = NULL;
  char timestamp[8000] = {0};        // proxy will close after this amount of time
  char timestamp_format[8003] = {0};        // proxy will close after this amount of time
  time(&timeRN);
  timeinfo = localtime(&timeRN);
  int timestamp_len = strftime(timestamp, sizeof(timestamp), "%m-%d-%Y %H:%M:%S", timeinfo);
  timestamp_len = sprintf(timestamp_format, "[%s] ", timestamp);

#ifdef LOGLIST_PRINTF
  #include <stdio.h>
  fprintf(stdout, "%.*s %.*s", timestamp_len - 1, timestamp_format, message_len, message);
  fflush(stdout);
#endif
#ifdef LOGLIST_SEQUENTIAL
  #include <stdio.h>
  fprintf(LOGLIST_SEQUENTIAL_FD, "%.*s %.*s", timestamp_len - 1, timestamp_format, message_len, message);
  fflush(LOGLIST_SEQUENTIAL_FD);
  return;
#endif
  pthread_mutex_lock(&logger->lock);
  for (int i = 0; i < timestamp_len; i++) {
    if (logger->tail->used_chars == LOGLIST_MAXBUF) {
      if (logger->tail->next == NULL) {
        logger->tail->next = calloc(sizeof(*logger->tail), 1);
      }
      logger->tail = logger->tail->next;
    }
    logger->tail->info[logger->tail->used_chars] = timestamp_format[i];
    logger->tail->used_chars += 1;
  }

  for (int i = 0; i < message_len; i++) {
    if (logger->tail->used_chars == LOGLIST_MAXBUF) {
      if (logger->tail->next == NULL) {
        logger->tail->next = calloc(sizeof(*logger->tail), 1);
      }
      logger->tail = logger->tail->next;
    }
    logger->tail->info[logger->tail->used_chars] = message[i];
    logger->tail->used_chars += 1;
  }
  logger->dirty = 1;
  pthread_mutex_unlock(&logger->lock);
}

struct LogList* init_loglist()
{
  struct LogList *current = calloc(sizeof(*current), 1);
  init_loglist2(current);
  return current;
}

void init_loglist2(struct LogList *init)
{
  init->tail = &init->head;
  pthread_mutex_init(&init->lock, NULL);
}

void destroy_loglist(struct LogList *head)
{
  struct LogList *temp_list = NULL; 
  struct LogNode *temp_node = NULL; 
  while (head != NULL) {
    struct LogNode *node_head = head->head.next; // skip the head because the first node is a member of the struct.
    while (node_head != NULL) {
      temp_node = node_head;
      node_head = node_head->next;
      free(temp_node);
    }
    temp_list = head;
    head = head->next;
    pthread_mutex_destroy(&temp_list->lock);
    free(temp_list);
  }
}

int get_messages(struct LogListVisitor *visitor, int *len, char **message)
{
  if (!visitor->current_list) {
    return 0;
  }

  if (!visitor->current_list->dirty) {
    *len = 0;
    *message = visitor->current_list->head.info;
    visitor->current_list = visitor->current_list->next;
    return visitor->current_list != NULL;
  }

  pthread_mutex_lock(&visitor->current_list->lock);

  if (!visitor->current_node) {
    visitor->current_node = &visitor->current_list->head;
  }

  if (visitor->current_node->used_chars == 0) {
    visitor->current_node = visitor->current_node->next;
    pthread_mutex_unlock(&visitor->current_list->lock);
    *len = 0;
    *message = visitor->current_list->head.info;
    return 1;
  }

  *len = visitor->current_node->used_chars;
  *message = visitor->current_node->info;

  visitor->current_node->used_chars = 0;

  visitor->current_node = visitor->current_node->next;
  pthread_mutex_unlock(&visitor->current_list->lock);
  if (!visitor->current_node) {
    visitor->current_list->dirty = 0;
    visitor->current_list = visitor->current_list->next;
  }

  return 1;
}
#endif
