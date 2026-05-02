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
    my_dlopen() : opens a shared library and returns a handle

    For challenge 2, we:
      - allocate a dl_handle structure,
      - open the file and validate its ELF header (via elf_open_and_check_ehdr),
      - return the handle to the caller (NULL if error).
*/
void *my_dlopen(const char *path)
{
    if (!path) {
        fprintf(stderr, "my_dlopen: path is null\n");
        return NULL;
    }

    // allocate using our handle structure (-> calloc initializes all fields to 0 + safer)
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

    // chall_4 : mapping PT_LOAD segments into memory
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
    e_entry to find the symbol table 
     
    relocations (chall 5) have already fixed the name/addr pointers in my_symbols -> we simply read: base_addr + e_entry
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

    // cast 
    struct dl_handle *h = (struct dl_handle *)opaque;
    struct my_symbol *sym = (struct my_symbol *)h->symtab;

    if (!sym) {
        fprintf(stderr, "my_dlsym: no symbol table in handle\n");
        return NULL;
    }

    // iterate over the my_symbols table, compare each name, and return their real address after loading and relocation
    for (int i = 0; sym[i].name != NULL; i++) {
        if (strcmp(sym[i].name, name) == 0) {
            fprintf(stderr, "my_dlsym: '%s' found at %p\n", name, sym[i].addr);
            return sym[i].addr;
        }
    }

    fprintf(stderr, "my_dlsym: symbol '%s' not found\n", name);
    return NULL;
}