# wprintidle-c

A reimagining of xprintidle for Wayland environments. This tool reports user idle time on Wayland compositors using the ext-idle-notify-v1 protocol.

## Overview

`wprintidle-c` reports user idle time on Wayland desktops, similar to how `xprintidle` works on X11. It consists of two components:

- **wprintidle-c-daemon**: A background service that maintains a connection to the Wayland compositor and tracks idle state
- **wprintidle-c**: A lightweight client that queries the daemon and prints the current idle time

This daemon/client architecture allows you to query idle time on demand (just like xprintidle) while the daemon handles the Wayland protocol requirements in the background.

## Inspiration

This project was inspired by [wprintidle](https://codeberg.org/andyscott/wprintidle) by Andy Scott, which was originally written in Zig, and licensed under GPLv3.

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

This installs both binaries (`wprintidle-c-daemon` and `wprintidle-c`) to `/usr/local/bin/` and a systemd unit file to `/usr/local/lib/systemd/user/`.

## Usage

### Starting the Daemon

The daemon must be running before you can query idle time. Start it using systemd (recommended) or manually:

```bash
# Using systemd (recommended)
systemctl --user enable wprintidle-c.service --now

# Or manually
wprintidle-c-daemon [timeout_ms] &
```

The optional `timeout_ms` parameter specifies how long (in milliseconds) before the user is considered idle. Default is 1000ms (1 second).

### Querying Idle Time

Once the daemon is running, query the idle time using the client:

```bash
wprintidle-c           # Print idle time in seconds (default)
wprintidle-c -s        # Print idle time in seconds (explicit)
wprintidle-c -m        # Print idle time in milliseconds
wprintidle-c --help    # Show help message
```

The client connects to the daemon, queries the idle time, prints it, and exits immediately.

### Integration Examples

**In shell scripts:**
```bash
if [ $(wprintidle-c) -gt 300 ]; then
    echo "User has been idle for more than 5 minutes"
fi
```

**For Emacs org-mode:**
```elisp
(setq org-clock-x11idle-program-name "wprintidle-c")
```

## How It Works

**Daemon:**
1. Connects to the Wayland display
2. Binds to the `wl_seat` and `ext_idle_notifier_v1` interfaces
3. Creates an idle notification with the specified timeout
4. Listens for idle/resumed events from the compositor
5. Runs a Unix domain socket server at `$XDG_RUNTIME_DIR/wprintidle-c.sock`
6. Multiplexes between Wayland events and client requests using `select()`

**Client:**
1. Connects to the daemon's Unix socket
2. Sends a query command (seconds or milliseconds)
3. Receives and prints the response
4. Exits

## Differences from xprintidle

- **Daemon requirement**: A background daemon is needed because Wayland compositors push idle events rather than allowing on-demand queries
- **Client/server architecture**: The daemon maintains the Wayland connection while the client provides a simple query interface
- **Configurable timeout**: Specify when the user should be considered idle (daemon parameter)

## Troubleshooting

**"Error: wprintidle-c daemon not running"**
```bash
# Check if daemon is running
systemctl --user status wprintidle-c.service

# View daemon logs
journalctl --user -u wprintidle-c.service -f

# Start the daemon
systemctl --user start wprintidle-c.service
```

**Verify socket exists:**
```bash
ls -l $XDG_RUNTIME_DIR/wprintidle-c.sock
```

**Test daemon manually:**
```bash
# Stop systemd service first
systemctl --user stop wprintidle-c.service

# Run daemon in foreground to see output
wprintidle-c-daemon
```

## License

GPL v3 - See the source files for details.

## Contributing

Contributions are welcome! Feel free to submit issues or pull requests.
