# Makefile

.PHONY: build
build: server client

server: mosh-udp-server.c
	$(CC) -W -Wall -o $@ $^ -static

client: mosh-udp-client.c
	$(CC) -W -Wall -o $@ $^ -static

.PHONY: clean
clean:
	rm -rf client server

