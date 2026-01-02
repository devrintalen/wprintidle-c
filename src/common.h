#ifndef COMMON_H
#define COMMON_H

// Protocol command constants
#define CMD_QUERY_SECONDS "QUERY_SECONDS\n"
#define CMD_QUERY_MILLIS "QUERY_MILLIS\n"
#define MAX_RESPONSE_LEN 32

// Socket path resolution
const char* get_socket_path(void);

#endif // COMMON_H
