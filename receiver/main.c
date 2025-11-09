/**
 * main.c
 *
 * Entrypoint for the receiver/analyzer program. This binary starts the
 * receive-and-process pipeline by calling the higher-level run_receiver()
 * implementation (see `common.c` / `io.c`).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "io.h"

/**
 * Program entrypoint.
 *
 * This function forwards to run_receiver() which performs socket creation,
 * thread startup and the main receive loop.
 *
 * @param argc count of command-line arguments (unused)
 * @param argv array of command-line arguments (unused)
 * @return return code from run_receiver()
 */
int main(int argc, char **argv){
  (void)argc; (void)argv;
  return run_receiver();
}
