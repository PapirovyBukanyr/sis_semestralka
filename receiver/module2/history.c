#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* History file path (kept consistent with existing code) */
#define LOG_HISTORY_FILE "data/log_history.bin"

history_entry_t *history_load_all(size_t *out_n){
    *out_n = 0;
    FILE *f = fopen(LOG_HISTORY_FILE, "rb");
    if(!f) return NULL;
    if(fseek(f, 0, SEEK_END) != 0){ fclose(f); return NULL; }
    long sz = ftell(f);
    if(sz <= 0){ fclose(f); return NULL; }
    size_t count = (size_t)sz / sizeof(history_entry_t);
    if(fseek(f, 0, SEEK_SET) != 0){ fclose(f); return NULL; }
    history_entry_t *arr = malloc(sizeof(history_entry_t) * count);
    if(!arr){ fclose(f); return NULL; }
    if(fread(arr, sizeof(history_entry_t), count, f) != count){ free(arr); fclose(f); return NULL; }
    fclose(f);
    *out_n = count;
    return arr;
}

int history_append(const history_entry_t *e){
    if(!e) return -1;
    FILE *f = fopen(LOG_HISTORY_FILE, "ab");
    if(!f) return -1;
    size_t written = fwrite(e, sizeof(*e), 1, f);
    fclose(f);
    return written == 1 ? 0 : -1;
}
