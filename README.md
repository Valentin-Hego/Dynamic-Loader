# ISOS Loader

ISOS Loader is a C project focused on understanding how a Linux dynamic loader works internally.

The goal was to build our own loader capable of loading and executing functions from an ELF shared library (`.so`) without using the standard `dlopen()` and `dlsym()` functions.

During the project, I worked directly with ELF files, memory mapping, `PT_LOAD` segments, relocations and symbol resolution. The loader parses the library, maps its segments into memory, applies the required relocations and finally resolves the functions that can be executed.

I also implemented an additional obfuscation feature that embeds an XOR-encrypted version of the shared library directly inside the executable. At runtime, the library is decrypted and loaded from memory using `memfd_create()`.

This project gave me a better understanding of **Linux internals, ELF binaries, virtual memory and low-level C programming**, while also requiring careful debugging and the use of tools such as GCC static analysis, Clang-Tidy and GitLab CI.


Utilisation :
## Cas 1 : payload embarquée (pas de lib en argument)
./isos_loader foo_exported bar_exported

## Cas 2 : lib explicite en CLI (écrase le payload)
./isos_loader src/libfoo.so foo_exported bar_exported

