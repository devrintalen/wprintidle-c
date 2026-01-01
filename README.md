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
make
```

This will:
1. Download the ext-idle-notify-v1 protocol
2. Generate protocol headers and source files
3. Compile the executable

## Installation

```bash
sudo make install
```

This installs the binary to `/usr/local/bin/wprintidle-c`.

## Usage

Start the daemon in the background:

```bash
wprintidle-c [timeout_ms] &
```

- `timeout_ms`: Time in milliseconds before considering user idle (default: 1000)

The program will print its PID when started. You can then query the idle time by sending signals:

### Query Idle Time in Seconds

```bash
kill -SIGUSR1 <PID>
```

### Query Idle Time in Milliseconds

```bash
kill -SIGUSR2 <PID>
```

### Example

```bash
# Start the daemon
$ wprintidle-c 5000 &
wprintidle-c started (PID: 12345)
Send SIGUSR1 for idle time in seconds, SIGUSR2 for milliseconds

# Query idle time in seconds
$ kill -SIGUSR1 12345
42

# Query idle time in milliseconds
$ kill -SIGUSR2 12345
42731
```

## How It Works

1. Connects to the Wayland display
2. Binds to the wl_seat and ext_idle_notifier_v1 interfaces
3. Creates an idle notification with the specified timeout
4. Listens for idle/resumed events from the compositor
5. Responds to SIGUSR1/SIGUSR2 signals by printing current idle time

## Differences from xprintidle

- **Must run as a daemon**: Wayland doesn't allow on-demand idle time queries
- **Signal-based interface**: Use signals to query idle time from the running process
- **Configurable timeout**: Specify when the user should be considered idle

## Compositor Support

This tool requires a Wayland compositor that implements the ext-idle-notify-v1 protocol, such as:
- Sway
- Hyprland
- wlroots-based compositors

## License

GPL v3 - See the source files for details.

## Contributing

Contributions are welcome! Feel free to submit issues or pull requests.
