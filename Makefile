SRC=src/
BIN=output/

CPPFLAGS=-pthread -std=c++11
CC = g++

.PHONY: clean

all: $(BIN) server client

server:
	$(CC) $(CPPFLAGS) -o $(BIN)server $(SRC)server.cpp

client:
	$(CC) $(CPPFLAGS) -o $(BIN)client $(SRC)client.cpp

clean:
	rm $(BIN)*