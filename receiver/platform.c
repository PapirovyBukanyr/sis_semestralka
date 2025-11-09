/*
 * platform.c
 *
 * Platform abstraction helpers (timing, sleep, platform-specific small
 * utilities) used by the receiver.
 */

#ifndef PLATFORM_C_HEADER
#define PLATFORM_C_HEADER
#include "platform.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/**
 * Initialize platform-specific socket subsystem.
 *
 * @return 0 on success, non-zero on failure
 */
int platform_socket_init(void) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return -1;
    }
#endif
    return 0;
}

/**
 * Clean up platform-specific socket subsystem.
 */
void platform_socket_cleanup(void){
#ifdef _WIN32
    WSACleanup();
#endif
}
