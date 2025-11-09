/*
 * represent.c
 *
 * Converts processed network outputs into human-readable or transmittable
 * representations and forwards them to the output queues or logging.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common.h"
#include "../queues.h"
#include "../log.h"

/**
 * Representation thread.
 *
 * Reads formatted strings from `repr_queue` and writes them to the log for
 * human-readable representation. Returns when the queue is closed.
 *
 * @param arg unused thread argument
 */
void* represent_thread(void* arg){
    (void)arg;
    double last_target_first = 0.0;
    int have_last_target = 0;

    while(1){
        char *line = queue_pop(&repr_queue);
        if(!line) break;
        LOG_INFO("[represent] %s\n", line);
        char *tpos = strstr(line, "target,");
        if(tpos){
            char *numstart = tpos + strlen("target,");
            while(*numstart == ' ') numstart++;
            char *endptr = NULL;
            double v = strtod(numstart, &endptr);
            if(endptr != numstart){
                last_target_first = v;
                have_last_target = 1;
            }
        }
        if(have_last_target && strncmp(line, "pred,", 5) == 0){
            char *first_comma = strchr(line, ',');
            if(first_comma){
                char *tok = first_comma + 1;
                while(*tok == ' ') tok++;
                char *endptr = NULL;
                double pred = strtod(tok, &endptr);
                if(endptr != tok){
                    if(pred > last_target_first && (pred - last_target_first) > 100000.0){
                        LOG_ERROR("\x1b[31mHIGH RISK OF APPROACHING ANOMALIES: last_target=%.6f, prediction=%.6f, diff=%.6f\x1b[0m\n",
                                  last_target_first, pred, pred - last_target_first);
                    }
                }
            }
        }

        free(line);
    }
    return NULL;
}
