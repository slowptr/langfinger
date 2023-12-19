CC = gcc
CFLAGS = -Wall -m32 -march=x86-64 -std=c99
LDFLAGS =
SRCS = main.c sqlite3.c
TARGET = langfinger.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)
