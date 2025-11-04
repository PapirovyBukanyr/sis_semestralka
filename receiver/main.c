/*
 Receiver / Analyzer program.
 Listens on UDP port 9000, enqueues raw logs to raw_queue.
 Spawns threads:
  - preproc_thread (module1)
  - nn_thread (module2)
  - represent_thread (module3)
  - ui_thread (module4)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Move platform/socket and JSON helpers to dedicated files for clarity */
#include "platform.h"
#include "io.h"

int main(int argc, char **argv){
    (void)argc; (void)argv;
    return run_receiver();
}
