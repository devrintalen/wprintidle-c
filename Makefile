CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lwayland-client

TARGET = wprintidle-c
SRC_DIR = src
PROTOCOL_DIR = protocol

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

uninstall:
	rm -f $(DESTDIR)/usr/local/bin/$(TARGET)
