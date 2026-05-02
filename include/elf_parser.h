#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <elf.h>
#include <stdint.h>
#include <stddef.h>

/*
    struct to store the handle returned by my_dlopen()
*/
struct dl_handle {
    int fd;  // file descriptor of the .so
    Elf64_Ehdr ehdr; // header copy of the executable

    // CHALL_3
    Elf64_Phdr *load_segs; // array of PT_LOAD segments
    size_t load_count; // number of PT_LOAD found
    uint64_t mem_size; // memory size

    // chall 4
    void *base_addr;

    //chall 6
    void *symtab;   // points to my_symbols
};


// Elf64_Ehdr -> type from elf.h


/*
    opens the shared library at path, reads its ELF header into handle->ehdr
    and performs the checks from chall_2
 */
 int elf_open_and_check_ehdr(const char *path, struct dl_handle *handle);


/*
    finds the PT_LOAD segments -> checks from chall_3 and fills the handle
 */
int elf_find_load_segments(struct dl_handle *handle);

/*
    maps the PT_LOAD segments into memory and fills handle->base_addr
 */
int seg_load_mem(struct dl_handle *handle);

/*
    relocate the symbols by looking into the .rela.dyn section
 */
int relocations(struct dl_handle *handle);

#endif /* ELF_PARSER_H */