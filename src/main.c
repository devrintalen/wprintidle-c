#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <wayland-client.h>
#include "ext-idle-notify-v1-client-protocol.h"

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

static void handle_sigusr1(int sig) {
    (void)sig;
    uint64_t idle_ms = get_idle_time_ms();
    printf("%lu\n", idle_ms / 1000);
    fflush(stdout);
}

static void handle_sigusr2(int sig) {
    (void)sig;
    uint64_t idle_ms = get_idle_time_ms();
    printf("%lu\n", idle_ms);
    fflush(stdout);
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

    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    printf("wprintidle-c started (PID: %d)\n", getpid());
    printf("Send SIGUSR1 for idle time in seconds, SIGUSR2 for milliseconds\n");
    fflush(stdout);

    while (wl_display_dispatch(state.display) != -1) {
        /* Event loop */
    }

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
