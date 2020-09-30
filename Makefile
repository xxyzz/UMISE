CC = clang
CFLAGS = -Wall -Wextra
TARGETS = unpackpak

all: $(TARGETS)

unpackpak: unpackpak.c
	$(CC) $< -o $@ $(CFLAGS)

clean:
	rm -rf $(TARGETS) ./*.dSYM
