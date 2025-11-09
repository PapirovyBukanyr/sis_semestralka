/**
 * platform.h
 *
 * Platform abstraction helpers (timing, sleep, platform-specific small utilities) used by the receiver.
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <sys/time.h>
  #include <unistd.h>
#endif

#ifdef _WIN32
  typedef SOCKET socket_t;
  typedef int socklen_t;
  #define CLOSESOCKET(s) closesocket(s)
  #ifndef INVALID_SOCKET
    #define INVALID_SOCKET (SOCKET)(~0)
  #endif
#else
  typedef int socket_t;
  #define CLOSESOCKET(s) close(s)
  #ifndef INVALID_SOCKET
    #define INVALID_SOCKET -1
  #endif
#endif

int platform_socket_init(void);
void platform_socket_cleanup(void);

#endif
