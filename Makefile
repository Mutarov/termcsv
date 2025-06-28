CC ?= gcc

CFLAGS += -Wall -Wextra -O2 -pedantic -std=c99
LDFLAGS += -lncursesw

TARGET = app

SRC = main.c
OBJ = $(SRC:.c=.o)

.PHONY: all check clean

all: check $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
