CC = gcc
CFLAGS = -L. -I./thirdparty -Wall -m32 -march=x86-64 -std=c99
LDFLAGS = -lssl -lcrypto
SRCS = main.c sqlite3.c
TARGET = langfinger.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)
