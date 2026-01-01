# wprintidle-c

A C implementation of xprintidle for Wayland environments. This tool reports user idle time on Wayland compositors using the ext-idle-notify-v1 protocol.

## Overview

`wprintidle-c` monitors user idle time on Wayland desktops, similar to how `xprintidle` works on X11. Unlike xprintidle which can query idle time on demand, wprintidle-c must remain running as a background process because Wayland compositors control all idle event handling.

## Inspiration

This project is a C port of [wprintidle](https://codeberg.org/andyscott/wprintidle) by Andy Scott, which was originally written in Zig, and licensed under GPLv3.

## Requirements

- Wayland compositor with ext-idle-notify-v1 protocol support
- libwayland-client
- wayland-scanner (for building)
- gcc or compatible C compiler

## Building

```bash
make clean && make
```

This will:
1. Generate protocol headers and source files
2. Compile the executable

## Installation

```bash
make install
```

This installs the binary to `/usr/local/bin/wprintidle-c` and a systemd unit file to `/usr/local/lib/systemd/user/`.

## Usage

The program has to run continuously to report idle time, unlike `xprintidle`. It can be started manually or by using the supplied systemd unit file:


```bash
# Manual
wprintidle-c [timeout_ms] &
# Systemd
systemctl --user enable wprintidle-c.service --now
```

The program will print its PID when started. You can then query the idle time by sending signals:

```bash
pkill -SIGUSR1 wprintidle-c # seconds
pkill -SIGUSR2 wprintidle-c # milliseconds
```

Remember that if you are doing this interactively, that your idle time will most always be `0`. You can set up a bash `while` loop or similar to run the command without your input.

If you need this utility for Emacs org-mode (as I do), you can incorporate it into your `init.el` like so:

```elisp
(org-clock-x11idle-program-name "pkill -SIGUSR1 wprintidle-c")
```

## How It Works

1. Connects to the Wayland display
2. Binds to the `wl_seat` and `ext_idle_notifier_v1` interfaces
3. Creates an idle notification with the specified timeout
4. Listens for idle/resumed events from the compositor
5. Responds to `SIGUSR1`/`SIGUSR2` signals by printing current idle time

## Differences from xprintidle

- **Must run as a daemon**: Wayland doesn't allow on-demand idle time queries
- **Signal-based interface**: Use signals to query idle time from the running process
- **Configurable timeout**: Specify when the user should be considered idle

## License

GPL v3 - See the source files for details.

## Contributing

Contributions are welcome! Feel free to submit issues or pull requests.
