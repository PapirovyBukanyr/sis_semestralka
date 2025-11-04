#include <stdio.h>
#include "platform.h"
#ifdef _WIN32
#include <windows.h>
#endif

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

void platform_socket_cleanup(void){
#ifdef _WIN32
    WSACleanup();
#endif
}
