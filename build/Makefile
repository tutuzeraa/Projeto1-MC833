CC = gcc

CFLAGS = -Wall -g

LDFLAGS = -lsqlite3 -pthread

SERVER_TARGET = server 
CLIENT_TARGET = client 

SRC_DIR = ../src

SERVER_SOURCE = $(SRC_DIR)/server.c
CLIENT_SOURCE = $(SRC_DIR)/client.c

SERVER_OBJECT = $(notdir $(SERVER_SOURCE:.c=.o))
CLIENT_OBJECT = $(notdir $(CLIENT_SOURCE:.c=.o))

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_OBJECT)
	$(CC) $(SERVER_OBJECT) -o $(SERVER_TARGET) $(LDFLAGS)

$(CLIENT_TARGET): $(CLIENT_OBJECT)
	$(CC) $(CLIENT_OBJECT) -o $(CLIENT_TARGET) 

%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJECTS) $(CLIENT_OBJECTS) $(SERVER_TARGET) $(CLIENT_TARGET)

.PHONY: all clean
