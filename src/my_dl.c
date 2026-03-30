#include "my_dl.h"
#include "elf_parser.h"

#include <stdio.h>
#include <stdlib.h>

/*
    my_dlopen() : ouvre une bibliothèque partagée et retourne un handle

    Pour le challenge 2, on :
      - alloue une structure dl_handle,
      - ouvre le fichier et valide son en-tête ELF (via elf_open_and_check_ehdr),
      - retourne le handle à l’appelant (NULL si error).
*/
void *my_dlopen(const char *path)
{
    if (!path) {
        fprintf(stderr, "my_dlopen: path is null\n");
        return NULL;
    }

    // alloue avec notre stucture de handle (-> calloc tous les champs a 0 + sur)
    struct dl_handle *handle = calloc(1, sizeof(*handle));
    if (!handle) {
        fprintf(stderr, "my_dlopen: out of memory\n");
        return NULL;
    }
    handle->fd = -1;

    // les checks
    if (elf_open_and_check_ehdr(path, handle) < 0) {
        free(handle);
        return NULL;
    }

    return handle;
}

/*
    my_dlsym() : cherche un symbol avec son nom dans la librairy chargée.
    pas encore faite (-> chall_6)
 */
void *my_dlsym(void *handle, const char *name)
{
    if (!handle || !name) {
        fprintf(stderr, "my_dlsym: invalid arguments\n");
        return NULL;
    }

    fprintf(stderr, "my_dlsym: not yet implemented\n");
    return NULL;
}