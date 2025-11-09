#include <stdio.h>

#include "platform.h"

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * Initialize platform-specific socket subsystem.
 *
 * On Windows this initializes Winsock (WSAStartup). On POSIX systems this is a
 * no-op and returns success.
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
 *
 * On Windows this calls WSACleanup(). On POSIX systems this is a no-op.
 */
void platform_socket_cleanup(void){
#ifdef _WIN32
    WSACleanup();
#endif
}
