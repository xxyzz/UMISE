CC = clang
CFLAGS = -Wall -Wextra
TARGETS = extractpak

all: $(TARGETS)

extractpak: extractpak.c
	$(CC) $< -o $@ $(CFLAGS)

clean:
	rm -rf $(TARGETS) ./*.dSYM
