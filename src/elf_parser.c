#include "elf_parser.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
    Lis le ELF header du fichier et fait les checks du chall_2 :
 
    checks :
    1: Le fichier commence par les octets magiques ELF (\x7fELF)
    2: Le binaire est en 64 bits (EI_CLASS == ELFCLASS64)
    3: Le binaire est un objet partagé / une bibliothèque dynamique (e_type == ET_DYN)
    4: La taille de notre structure Elf64_Ehdr correspond au champ e_ehsize
    5: Le nombre d’en-têtes de programme (e_phnum) est strictement supérieur à 0
*/


int elf_open_and_check_ehdr(const char *path, struct dl_handle *handle){
    // ouvre la shared lib en read_only
    handle->fd = open(path, O_RDONLY);
    if (handle->fd < 0) {
        fprintf(stderr, "elf_open_and_check_ehdr: cannot open '%s'\n", path);
        return -1;
    }

    // Lire exactement sizeof(Elf64_Ehdr) octets dans notre structure
    ssize_t n = read(handle->fd, &handle->ehdr, sizeof(handle->ehdr));
    if (n < 0 || (size_t)n < sizeof(handle->ehdr)) {
        fprintf(stderr, "elf_open_and_check_ehdr: failed to read ELF header from '%s'\n", path);
        close(handle->fd);
        handle->fd = -1;
        return -1;
    }

    // check 1
    if (memcmp(handle->ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "elf_open_and_check_ehdr: '%s' is not an ELF file\n", path);
        close(handle->fd);
        handle->fd = -1;
        return -1;
    }

    // check 2
    if (handle->ehdr.e_ident[EI_CLASS] != ELFCLASS64) {
        fprintf(stderr, "elf_open_and_check_ehdr: '%s' is not a 64-bit ELF\n", path);
        close(handle->fd);
        handle->fd = -1;
        return -1;
    }

    // check 3
    if (handle->ehdr.e_type != ET_DYN) {
        fprintf(stderr, "elf_open_and_check_ehdr: '%s' is not a dynamic library (ET_DYN)\n", path);
        close(handle->fd);
        handle->fd = -1;
        return -1;
    }

    // check 4
    if (sizeof(handle->ehdr) != handle->ehdr.e_ehsize) {
        fprintf(stderr,
                "elf_open_and_check_ehdr: ehdr size mismatch in '%s' "
                "(expected %u, got %zu)\n",
                path, handle->ehdr.e_ehsize, sizeof(handle->ehdr));
        close(handle->fd);
        handle->fd = -1;
        return -1;
    }

    // check 5
    if (handle->ehdr.e_phnum == 0) {
        fprintf(stderr, "elf_open_and_check_ehdr: '%s' has no program headers\n", path);
        close(handle->fd);
        handle->fd = -1;
        return -1;
    }

    // tout s'est bien passé
    fprintf(stderr,
            "elf_open_and_check_ehdr: '%s' success "
            "(%u segment found)\n",
            path, handle->ehdr.e_phnum);
    return 0;
}
