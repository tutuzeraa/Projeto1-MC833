CC = gcc

CFLAGS = -Wall -g

LDFLAGS = -lsqlite3

TARGET = server 

SRC_DIR = ../src

SOURCES = $(SRC_DIR)/server.c

OBJECTS = $(notdir $(SOURCES:.c=.o))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
