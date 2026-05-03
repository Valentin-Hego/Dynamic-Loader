# This Makefile is for the ISOS project and ensure compatibility with the CI.
# Make sure to include this file in your root Makefile (i.e., at the top-level of your repository).

# Headers directory
INCLUDE_DIR = ./include

# Loader source files (used by the CI)
SRC_FILES = ./src/my_dl.c ./src/isos_loader.c ./src/elf_parser.c

# ─── Compilation settings ──────────────────────────────────────────────
CC      = gcc
CFLAGS  = -Wall -Wextra -Wuninitialized -Wpointer-arith -Wcast-qual -Wcast-align \
          -I$(INCLUDE_DIR)

# ─── Targets ──────────────────────────────────────────────────────────────────
.PHONY: all clean clang-check
all: encrypt src/libfoo.so isos_loader


# Compilation of the shared library (modified for chall6)
# we compile the lib with -Wl,-e,my_symbols so that e_entry points to our custom symbol table
src/libfoo.so: src/libfoo.c
	$(CC) -nostdlib -fPIC -shared -fvisibility=hidden \
		-Wl,-e,my_symbols \
		-o $@ $<

# bonus 2 encryption tool
encrypt: src/encrypt.c
	$(CC) -Wall -Wextra -o $@ $<

# Main program isos_loader
# compile normaly, then inject (at the end) the encrypted
isos_loader: src/isos_loader.c src/my_dl.c src/elf_parser.c src/libfoo.so encrypt
	$(CC) $(CFLAGS) -rdynamic -o $@ src/isos_loader.c src/my_dl.c src/elf_parser.c -ldl
	@echo "-----bonus 2 (encryption)--------"
	./encrypt src/libfoo.so src/encrypted.so
	cat src/encrypted.so >> $@ 

# Clang
clang-check:
	clang -Wall -Wextra -Wuninitialized -Wpointer-arith -Wcast-qual -Wcast-align \
	-I$(INCLUDE_DIR) -rdynamic --analyze $(SRC_FILES) -ldl

clean:
	rm -f src/libfoo.so isos_loader encrypt src/encrypted.so src/encrypt