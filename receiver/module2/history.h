#ifndef MODULE2_HISTORY_H
#define MODULE2_HISTORY_H

#include <stdint.h>
#include <stdlib.h>

typedef struct {
    int64_t ts;
    float in0;
    float in1;
} history_entry_t;

/* Load all history entries from data/log_history.bin.
   Returns a malloc'd array and sets *out_n to count. Caller must free() the returned pointer.
   On error or empty file returns NULL and *out_n = 0. */
history_entry_t *history_load_all(size_t *out_n);

/* Append a single history entry to the history file. Returns 0 on success, -1 on failure. */
int history_append(const history_entry_t *e);

#endif /* MODULE2_HISTORY_H */
