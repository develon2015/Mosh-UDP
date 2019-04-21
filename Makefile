# Makefile

.PHONY: build
build: server client

server: mosh-udp-server.c
	$(CC) -W -Wall -o $@ $^

client: mosh-udp-client.c
	$(CC) -W -Wall -o $@ $^ 

.PHONY: deploy
deploy:
	cp ./server /usr/bin/moshd
	cp ./client /usr/bin/moshc

.PHONY: clean
clean:
	rm -rf client server

