# Makefile

.PHONY: ALL
ALL: bin/server bin/client | bin

bin:
	mkdir bin

bin/server: src/mosh-udp-server.c
	$(CC) -W -Wall -o $@ $^

bin/client: src/mosh-udp-client.c
	$(CC) -W -Wall -o $@ $^ 

.PHONY: deploy
deploy:
	sudo cp bin/server /usr/bin/moshd
	sudo cp bin/client /usr/bin/moshc

.PHONY: clean
clean:
	cd bin && rm -rf client server

