CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lwayland-client

TARGET = wprintidle-c
SRC_DIR = src
PROTOCOL_DIR = protocol
UNIT_FILE = wprintidle-c.service
MAN_PAGE = wprintidle-c.1

SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/ext-idle-notify-v1-protocol.c
OBJECTS = $(SOURCES:.c=.o)

PROTOCOL_XML = $(PROTOCOL_DIR)/ext-idle-notify-v1.xml
PROTOCOL_HEADER = $(SRC_DIR)/ext-idle-notify-v1-client-protocol.h
PROTOCOL_SOURCE = $(SRC_DIR)/ext-idle-notify-v1-protocol.c

.PHONY: all clean install uninstall protocols

all: protocols $(TARGET)

protocols: $(PROTOCOL_HEADER) $(PROTOCOL_SOURCE)

$(PROTOCOL_HEADER): $(PROTOCOL_XML)
	wayland-scanner client-header $< $@

$(PROTOCOL_SOURCE): $(PROTOCOL_XML)
	wayland-scanner private-code $< $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(PROTOCOL_HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f $(PROTOCOL_HEADER) $(PROTOCOL_SOURCE)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)
	install -Dm644 $(UNIT_FILE) $(DESTDIR)/usr/local/lib/systemd/user/$(UNIT_FILE)
	install -Dm644 $(MAN_PAGE) $(DESTDIR)/usr/local/share/man/man1/$(MAN_PAGE)
	@echo "To enable the service, run: systemctl --user enable wprintidle-c.service"

uninstall:
	rm -f $(DESTDIR)/usr/local/bin/$(TARGET)
	rm -f $(DESTDIR)/usr/local/lib/systemd/user/$(UNIT_FILE)
	rm -f $(DESTDIR)/usr/local/share/man/man1/$(MAN_PAGE)
