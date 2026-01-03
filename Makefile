CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lwayland-client

DAEMON_TARGET = wprintidle-c-daemon
CLIENT_TARGET = wprintidle-c
SRC_DIR = src
PROTOCOL_DIR = protocol
UNIT_FILE = wprintidle-c.service
MAN_PAGE = wprintidle-c.1

DAEMON_SOURCES = $(SRC_DIR)/daemon.c $(SRC_DIR)/common.c $(SRC_DIR)/ext-idle-notify-v1-protocol.c
CLIENT_SOURCES = $(SRC_DIR)/client.c $(SRC_DIR)/common.c

DAEMON_OBJECTS = $(DAEMON_SOURCES:.c=.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:.c=.o)

PROTOCOL_XML = $(PROTOCOL_DIR)/ext-idle-notify-v1.xml
PROTOCOL_HEADER = $(SRC_DIR)/ext-idle-notify-v1-client-protocol.h
PROTOCOL_SOURCE = $(SRC_DIR)/ext-idle-notify-v1-protocol.c

.PHONY: all clean install uninstall protocols

all: protocols $(DAEMON_TARGET) $(CLIENT_TARGET)

protocols: $(PROTOCOL_HEADER) $(PROTOCOL_SOURCE)

$(PROTOCOL_HEADER): $(PROTOCOL_XML)
	wayland-scanner client-header $< $@

$(PROTOCOL_SOURCE): $(PROTOCOL_XML)
	wayland-scanner private-code $< $@

$(DAEMON_TARGET): $(DAEMON_OBJECTS)
	$(CC) $(DAEMON_OBJECTS) $(LDFLAGS) -o $(DAEMON_TARGET)

$(CLIENT_TARGET): $(CLIENT_OBJECTS)
	$(CC) $(CLIENT_OBJECTS) -o $(CLIENT_TARGET)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(PROTOCOL_HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(DAEMON_OBJECTS) $(CLIENT_OBJECTS) $(DAEMON_TARGET) $(CLIENT_TARGET)
	rm -f $(PROTOCOL_HEADER) $(PROTOCOL_SOURCE)

install: $(DAEMON_TARGET) $(CLIENT_TARGET)
	install -Dm755 $(DAEMON_TARGET) $(DESTDIR)/usr/bin/$(DAEMON_TARGET)
	install -Dm755 $(CLIENT_TARGET) $(DESTDIR)/usr/bin/$(CLIENT_TARGET)
	install -Dm644 $(UNIT_FILE) $(DESTDIR)/usr/lib/systemd/user/$(UNIT_FILE)
	install -Dm644 $(MAN_PAGE) $(DESTDIR)/usr/share/man/man1/$(MAN_PAGE)
	@echo "To enable the service, run: systemctl --user enable wprintidle-c.service"

uninstall:
	rm -f $(DESTDIR)/usr/bin/$(DAEMON_TARGET)
	rm -f $(DESTDIR)/usr/bin/$(CLIENT_TARGET)
	rm -f $(DESTDIR)/usr/lib/systemd/user/$(UNIT_FILE)
	rm -f $(DESTDIR)/usr/share/man/man1/$(MAN_PAGE)
