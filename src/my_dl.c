#include "my_dl.h"
#include "elf_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <stdint.h>

struct my_symbol { // custom dynamic symbols table
    const char *name;
    void *addr;
};

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

    // chall 2
    if (elf_open_and_check_ehdr(path, handle) < 0) {
        free(handle);
        return NULL;
    }

    // challenge 3 : Find the PT_LOAD segments
    if (elf_find_load_segments(handle) < 0) {
        free(handle);
        return NULL;
    }

    // chall_4 : mapping des segments PT_LOAD en mémoire
    if (seg_load_mem(handle) < 0) {
        free(handle);
        return NULL;
    }

    // chall 5 : dynamic relocations
    if (relocations(handle)< 0) {
        free(handle);
        return NULL;
    }

    /* chall 6 : Call the exported symbols
    e_entry pour trouver la table des symboles 
     
    les relocations (chall 5) on deja corrigé les pointeurs name/addr dans my_symbols -> on lis simplement : base_addr + e_entry
    */
    if (handle->ehdr.e_entry == 0) {
        fprintf(stderr, "err -> no symbol table\n");
        return handle;
    }

    handle->symtab = (char *)handle->base_addr + handle->ehdr.e_entry;
    fprintf(stderr, "my_dlopen: symtab found at %p (e_entry=0x%lx)\n", handle->symtab, (unsigned long)handle->ehdr.e_entry);

    return handle;
}

void *my_dlsym(void *opaque, const char *name)
{
    if (!opaque || !name)
        return NULL;

    struct dl_handle *h = (struct dl_handle *)opaque;
    struct my_symbol *sym = (struct my_symbol *)h->symtab;

    if (!sym) {
        fprintf(stderr, "my_dlsym: no symbol table in handle\n");
        return NULL;
    }

    //parcour la table my_symbols, compare chaque name, et retourne leurs addr reel apres chargement et relocation
    for (int i = 0; sym[i].name != NULL; i++) {
        if (strcmp(sym[i].name, name) == 0) {
            fprintf(stderr, "my_dlsym: '%s' found at %p\n", name, sym[i].addr);
            return sym[i].addr;
        }
    }

    fprintf(stderr, "my_dlsym: symbol '%s' not found\n", name);
    return NULL;
}