#pragma once

#include <stdio.h>

/*
 * According to RFC 5424, the syslog size limit is 1024
 */
#define BUFFER_SIZE 1024

#define HASH_SIZE ((size_t) 101)

struct msg_entry {
    char msg[BUFFER_SIZE];
    struct msg_entry *next;
    size_t size;
    size_t count;
};

struct message_counter {
    struct msg_entry *entries[HASH_SIZE];
};

typedef struct message_counter message_counter_t;

void msgcount_init(message_counter_t *handle);

size_t msgcount_incr(message_counter_t *handle, const char *msg, size_t size);
