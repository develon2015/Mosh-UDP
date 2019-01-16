# Makefile

.PHONY: build
build: server client

server: mosh-udp-server.c
	$(CC) -W -Wall -o $@ $^

client: mosh-udp-client.c
	$(CC) -W -Wall -o $@ $^

.PHONY: clean
clean:
	rm -rf client server

