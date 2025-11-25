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
#ifdef OPENAI_ENABLED
#include "openai_client.h"
#endif

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
        /* Optionally ask OpenAI to interpret the line. This block is compiled
         * only when `OPENAI_ENABLED` is defined (Makefile: `USE_OPENAI=1`). */
#ifdef OPENAI_ENABLED
        char *llm_reply = openai_interpret_with_system(
            "You are an AI assistant. Please look at this data and say one sentence, about what is going on. Norm is export_bytes: 32640.250000, export_flows: 10.033334, export_packets: 64.343330, export_rtr: 0.050239, export_rtt: 16149.753906, export_srt: 71455.148438. If the data indicates stable internet connection, say: 'The internet connection looks stable for the next three minutes.' If it indicates instability, say: 'The internet connection may experience instability in the next three minutes.' If it indicates a high risk of approaching anomalies, say: 'HIGH RISK OF APPROACHING ANOMALIES DETECTED.'. If you cannot tell, say: 'The data is inconclusive regarding internet stability.'. If it will be around that values, say 'Speed of internet in next three minutes will be fast.' Only respond with one sentence. Maximum length of your response is 100 characters. Here is the data:\n",
            line,
            "o4-mini");
        if(llm_reply){
            LOG_INFO("[LLM] %s\n", llm_reply);
            free(llm_reply);
        }
#endif
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
