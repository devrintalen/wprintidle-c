#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

static char socket_path_buffer[256];

const char* get_socket_path(void) {
    // Try XDG_RUNTIME_DIR first
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");

    if (runtime_dir && runtime_dir[0] != '\0') {
        snprintf(socket_path_buffer, sizeof(socket_path_buffer),
                 "%s/wprintidle-c.sock", runtime_dir);
    } else {
        // Fallback to /tmp with UID
        snprintf(socket_path_buffer, sizeof(socket_path_buffer),
                 "/tmp/wprintidle-c-%d.sock", getuid());
    }

    return socket_path_buffer;
}
