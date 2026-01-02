/*
 * wprintidle-c - Common implementation for daemon and client
 * Copyright (C) 2026 Devrin Talen <devrin@fastmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
