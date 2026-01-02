/*
 * wprintidle-c - Client to query Wayland idle time from daemon
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
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include "common.h"

int main(int argc, char *argv[]) {
    int query_millis = 0;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--milliseconds") == 0) {
            query_millis = 1;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--seconds") == 0) {
            query_millis = 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [-s|--seconds] [-m|--milliseconds] [-h|--help]\n", argv[0]);
            printf("Query idle time from wprintidle-c daemon.\n\n");
            printf("Options:\n");
            printf("  -s, --seconds        Print idle time in seconds (default)\n");
            printf("  -m, --milliseconds   Print idle time in milliseconds\n");
            printf("  -h, --help           Show this help message\n");
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            return 1;
        }
    }

    // Connect to daemon socket
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        fprintf(stderr, "Error: Failed to create socket: %s\n", strerror(errno));
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, get_socket_path(), sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Error: wprintidle-c daemon not running\n");
        fprintf(stderr, "Start it with: systemctl --user start wprintidle-c.service\n");
        fprintf(stderr, "Or run directly: wprintidle-c-daemon\n");
        close(sock_fd);
        return 1;
    }

    // Send query
    const char *cmd = query_millis ? CMD_QUERY_MILLIS : CMD_QUERY_SECONDS;
    ssize_t written = write(sock_fd, cmd, strlen(cmd));
    if (written < 0) {
        fprintf(stderr, "Error: Failed to send query: %s\n", strerror(errno));
        close(sock_fd);
        return 1;
    }

    // Read response
    char response[MAX_RESPONSE_LEN];
    ssize_t n = read(sock_fd, response, sizeof(response) - 1);
    if (n > 0) {
        response[n] = '\0';
        printf("%s", response);  // Already includes newline
    } else if (n == 0) {
        fprintf(stderr, "Error: Daemon closed connection without response\n");
        close(sock_fd);
        return 1;
    } else {
        fprintf(stderr, "Error: Failed to read response: %s\n", strerror(errno));
        close(sock_fd);
        return 1;
    }

    close(sock_fd);
    return 0;
}
