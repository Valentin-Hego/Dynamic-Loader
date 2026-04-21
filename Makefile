# This Makefile is for the ISOS project and ensure compatibility with the CI.
# Make sure to include this file in your root Makefile (i.e., at the top-level of your repository).
#

# Répertoire des headers
INCLUDE_DIR = ./include

# Fichiers sources du loader (utilisés par la CI)
SRC_FILES = ./src/my_dl.c ./src/isos_loader.c ./src/elf_parser.c

# ─── Paramètres de compilation ──────────────────────────────────────────────
CC      = gcc
CFLAGS  = -Wall -Wextra -Wuninitialized -Wpointer-arith -Wcast-qual -Wcast-align \
          -I$(INCLUDE_DIR)

# ─── Cibles ──────────────────────────────────────────────────────────────────
.PHONY: all clean clang-check
all: src/libfoo.so isos_loader #src/env_setup

# Compilation de la bibliothèque partagée (modif chall6)
#on compile la lib avec -Wl,-e,my_symbols pour que e_entry pointe sur notre table de symboles custom
src/libfoo.so: src/libfoo.c
	$(CC) -nostdlib -fPIC -shared -fvisibility=hidden \
		-Wl,-e,my_symbols \
		-o $@ $<

# Programme de test de l'environment setup -> casse le chall6 donc commenté
#src/env_setup: src/env_setup.c src/libfoo.so
#	$(CC) $(CFLAGS) -o $@ $< -Lsrc -lfoo -Wl,-rpath,$(PWD)/src

# Programme principal isos_loader
isos_loader: src/isos_loader.c src/my_dl.c src/elf_parser.c
	$(CC) $(CFLAGS) -rdynamic -o $@ $^ -ldl

# Clang
clang-check:
	clang -Wall -Wextra -Wuninitialized -Wpointer-arith -Wcast-qual -Wcast-align \
	-I$(INCLUDE_DIR) -rdynamic --analyze $(SRC_FILES) -ldl
clean:
	rm -f src/libfoo.so src/env_setup isos_loader