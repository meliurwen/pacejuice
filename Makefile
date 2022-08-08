
default: build
all: default

build:
	$(CC) pulseACEjuice.c -o pulseACEjuice $(shell pkg-config libpulse --cflags --libs)
