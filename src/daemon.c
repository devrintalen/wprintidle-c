#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <errno.h>
#include <wayland-client.h>
#include "ext-idle-notify-v1-client-protocol.h"
#include "common.h"

#define MAX_CLIENTS 10

struct idle_state {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_seat *seat;
    struct ext_idle_notifier_v1 *idle_notifier;
    struct ext_idle_notification_v1 *idle_notification;
    uint64_t idle_start_time;
    uint32_t timeout_ms;
    int is_idle;
};

static struct idle_state state = {0};
static volatile int running = 1;

static void idle_notification_idled(void *data,
                                     struct ext_idle_notification_v1 *notification) {
    (void)notification;
    struct idle_state *state = data;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    state->idle_start_time = now - state->timeout_ms;
    state->is_idle = 1;
}

static void idle_notification_resumed(void *data,
                                       struct ext_idle_notification_v1 *notification) {
    (void)notification;
    struct idle_state *state = data;
    state->is_idle = 0;
    state->idle_start_time = 0;
}

static const struct ext_idle_notification_v1_listener idle_notification_listener = {
    .idled = idle_notification_idled,
    .resumed = idle_notification_resumed,
};

static void registry_global(void *data, struct wl_registry *registry,
                             uint32_t name, const char *interface,
                             uint32_t version) {
    (void)version;
    struct idle_state *state = data;

    if (strcmp(interface, wl_seat_interface.name) == 0) {
        state->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
    } else if (strcmp(interface, ext_idle_notifier_v1_interface.name) == 0) {
        state->idle_notifier = wl_registry_bind(registry, name,
                                                 &ext_idle_notifier_v1_interface, 1);
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry,
                                     uint32_t name) {
    (void)data;
    (void)registry;
    (void)name;
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

static uint64_t get_idle_time_ms(void) {
    if (!state.is_idle) {
        return 0;
    }

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return now - state.idle_start_time;
}

static void handle_client_request(int client_fd) {
    char buffer[64];
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) return;

    buffer[n] = '\0';

    uint64_t idle_ms = get_idle_time_ms();
    char response[MAX_RESPONSE_LEN];

    if (strcmp(buffer, CMD_QUERY_SECONDS) == 0) {
        snprintf(response, sizeof(response), "%lu\n", idle_ms / 1000);
    } else if (strcmp(buffer, CMD_QUERY_MILLIS) == 0) {
        snprintf(response, sizeof(response), "%lu\n", idle_ms);
    } else {
        return;  // Unknown command
    }

    ssize_t written = write(client_fd, response, strlen(response));
    (void)written;  // Suppress unused variable warning
}

static int setup_socket_server(void) {
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, get_socket_path(), sizeof(addr.sun_path) - 1);

    // Remove stale socket
    unlink(addr.sun_path);

    if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Failed to bind socket: %s\n", strerror(errno));
        close(sock_fd);
        return -1;
    }

    if (listen(sock_fd, 5) < 0) {
        fprintf(stderr, "Failed to listen on socket: %s\n", strerror(errno));
        close(sock_fd);
        unlink(addr.sun_path);
        return -1;
    }

    // Set owner-only permissions
    if (chmod(addr.sun_path, 0600) < 0) {
        fprintf(stderr, "Failed to set socket permissions: %s\n", strerror(errno));
        close(sock_fd);
        unlink(addr.sun_path);
        return -1;
    }

    return sock_fd;
}

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

static void cleanup_socket(void) {
    unlink(get_socket_path());
}

int main(int argc, char *argv[]) {
    uint32_t timeout_ms = 1000;

    if (argc > 1) {
        timeout_ms = atoi(argv[1]);
        if (timeout_ms == 0) {
            fprintf(stderr, "Usage: %s [timeout_ms]\n", argv[0]);
            fprintf(stderr, "  timeout_ms: Time in milliseconds before considering user idle (default: 1000)\n");
            return 1;
        }
    }

    // Setup signal handlers for clean shutdown
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);
    atexit(cleanup_socket);

    // Connect to Wayland
    state.display = wl_display_connect(NULL);
    if (!state.display) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return 1;
    }

    state.registry = wl_display_get_registry(state.display);
    wl_registry_add_listener(state.registry, &registry_listener, &state);

    wl_display_roundtrip(state.display);

    if (!state.seat) {
        fprintf(stderr, "No wl_seat found\n");
        return 1;
    }

    if (!state.idle_notifier) {
        fprintf(stderr, "ext_idle_notifier_v1 not supported by compositor\n");
        return 1;
    }

    state.timeout_ms = timeout_ms;
    state.idle_notification = ext_idle_notifier_v1_get_idle_notification(
        state.idle_notifier, timeout_ms, state.seat);

    ext_idle_notification_v1_add_listener(state.idle_notification,
                                           &idle_notification_listener, &state);

    // Setup socket server
    int socket_fd = setup_socket_server();
    if (socket_fd < 0) {
        fprintf(stderr, "Failed to setup socket server\n");
        return 1;
    }

    int wayland_fd = wl_display_get_fd(state.display);
    int client_fds[MAX_CLIENTS] = {0};

    printf("wprintidle-c daemon started (PID: %d)\n", getpid());
    printf("Socket: %s\n", get_socket_path());
    fflush(stdout);

    // Main event loop
    while (running) {
        // Dispatch pending Wayland events
        while (wl_display_prepare_read(state.display) != 0) {
            wl_display_dispatch_pending(state.display);
        }
        wl_display_flush(state.display);

        // Setup fd_set for select
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(wayland_fd, &readfds);
        FD_SET(socket_fd, &readfds);

        int maxfd = (wayland_fd > socket_fd) ? wayland_fd : socket_fd;

        // Add connected clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] > 0) {
                FD_SET(client_fds[i], &readfds);
                if (client_fds[i] > maxfd) maxfd = client_fds[i];
            }
        }

        int ret = select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (ret < 0) {
            if (errno == EINTR) {
                wl_display_cancel_read(state.display);
                continue;
            }
            fprintf(stderr, "select() failed: %s\n", strerror(errno));
            wl_display_cancel_read(state.display);
            break;
        }

        // Handle Wayland events
        if (FD_ISSET(wayland_fd, &readfds)) {
            wl_display_read_events(state.display);
            wl_display_dispatch_pending(state.display);
        } else {
            wl_display_cancel_read(state.display);
        }

        // Accept new connections
        if (FD_ISSET(socket_fd, &readfds)) {
            int client_fd = accept(socket_fd, NULL, NULL);
            if (client_fd >= 0) {
                // Find empty slot
                int added = 0;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (client_fds[i] == 0) {
                        client_fds[i] = client_fd;
                        added = 1;
                        break;
                    }
                }
                if (!added) {
                    // No slots available, close connection
                    close(client_fd);
                }
            }
        }

        // Handle client requests
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] > 0 && FD_ISSET(client_fds[i], &readfds)) {
                handle_client_request(client_fds[i]);
                close(client_fds[i]);
                client_fds[i] = 0;
            }
        }
    }

    // Cleanup
    printf("\nShutting down...\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] > 0) {
            close(client_fds[i]);
        }
    }
    close(socket_fd);

    if (state.idle_notification) {
        ext_idle_notification_v1_destroy(state.idle_notification);
    }
    if (state.idle_notifier) {
        ext_idle_notifier_v1_destroy(state.idle_notifier);
    }
    if (state.seat) {
        wl_seat_destroy(state.seat);
    }
    if (state.registry) {
        wl_registry_destroy(state.registry);
    }
    if (state.display) {
        wl_display_disconnect(state.display);
    }

    return 0;
}
