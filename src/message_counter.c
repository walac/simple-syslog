#define _GNU_SOURCE
#include <memory.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "error.h"
#include "message_counter.h"

#define P ((size_t) 131)
#define M ((size_t) (SIZE_MAX/4))

/*
 * Polynomial Rolling Hash Function
 */
static size_t hash(const char *s) {
    size_t power_of_p = 1;
    size_t hash_val = 0;

    for (const char *p = s; *p; ++p) {
        hash_val = (hash_val + *p * power_of_p) % M;
        power_of_p = (power_of_p * P) % M;
    }

    return hash_val % HASH_SIZE;
}

void msgcount_init(message_counter_t *handle)
{
    bzero(handle, sizeof *handle);
}

size_t msgcount_incr(message_counter_t *handle, const char *msg, size_t size)
{
    assert(size <= BUFFER_SIZE);
    const size_t index = hash(msg);

    struct msg_entry *entry = handle->entries[index];
    while (entry) {
        if (entry->size == size && !memcmp(entry->msg, msg, size)) {
            ++entry->count;
            break;
        }

        entry = entry->next;
    }

    if (!entry) {
        entry = CHK_P(malloc(sizeof(struct msg_entry)));
        memcpy(entry->msg, msg, size);
        entry->count = 1;
        entry->next = handle->entries[index];
        entry->size = size;
        handle->entries[index] = entry;
    }

    return entry->count;
}
