CC = gcc
CFLAGS = -O2 -Wall -Wextra -municode -mwindows -Iinclude
LDFLAGS = -lbcrypt -lcomctl32 -lshell32 -lole32 -loleaut32 -luuid -lshlwapi
WINDRES = windres

TARGET = win10_file_enc.exe
SRCS = src/main.c src/crypto.c src/vault.c src/scanner.c src/shredder.c src/renamer.c src/i18n.c src/service.c src/ui.c
OBJS = $(SRCS:.c=.o)
RC_SRC = src/resources.rc
RC_OBJ = src/resources.o

all: $(TARGET)

$(RC_OBJ): $(RC_SRC) assets/app.manifest assets/app.ico
	$(WINDRES) $(RC_SRC) -O coff -o $(RC_OBJ)

$(TARGET): $(OBJS) $(RC_OBJ)
	$(CC) $(CFLAGS) $(OBJS) $(RC_OBJ) $(LDFLAGS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(RC_OBJ) $(TARGET)

.PHONY: all clean
