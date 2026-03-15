# This Makefile is for the ISOS project and ensure compatibility with the CI.
# Make sure to include this file in your root Makefile (i.e., at the top-level of your repository).
#
# Répertoire des headers
INCLUDE_DIR = ./include

# Fichiers sources du loader (utilisés par la CI)
SRC_FILES = ./src/my_dl.c ./src/isos_loader.c

# ─── Paramètres de compilation ──────────────────────────────────────────────
CC      = gcc
CFLAGS  = -Wall -Wextra -Wuninitialized -Wpointer-arith -Wcast-qual -Wcast-align \
          -I$(INCLUDE_DIR)

# ─── Cibles ──────────────────────────────────────────────────────────────────
.PHONY: all clean

all: src/libfoo.so src/env_setup isos_loader

# Compilation de la bibliothèque partagée
src/libfoo.so: src/libfoo.c
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $<

# Programme de test de l'environment setup
src/env_setup: src/env_setup.c src/libfoo.so
	$(CC) $(CFLAGS) -o $@ $< -Lsrc -lfoo -Wl,-rpath,$(PWD)/src

# Programme principal isos_loader
isos_loader: src/isos_loader.c src/my_dl.c
	$(CC) $(CFLAGS) -rdynamic -o $@ $^ -ldl

clean:
	rm -f src/libfoo.so src/env_setup isos_loader
