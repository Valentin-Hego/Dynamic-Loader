#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <elf.h>
#include <stdint.h>
#include <stddef.h>

/*
    structure pour stocker de handle retourné par my_dlopen()
*/
struct dl_handle {
    int fd;  // file descriptor du .so
    Elf64_Ehdr ehdr; // copie du header de l'executable

    // CHALL_3
    Elf64_Phdr *load_segs; // tableau des segments PT_LOAD
    size_t load_count; // nombre de PT_LOAD trouvés
    uint64_t mem_size; // taille memoire

    // chall 4
    void *base_addr;

    //chall 6
    void *symtab;   // pointe vers my_symbols
};


// Elf64_Ehdr -> type de elf.h


/*
    ouvre la shared library au path, lis son ELF header into handle->ehdr
    et fait les vérifs du chall_2
 */
 int elf_open_and_check_ehdr(const char *path, struct dl_handle *handle);


 /*
     Trouve les PT_LOAD segments -> verifs du chall_3 et load le handle 
 */
 int elf_find_load_segments(struct dl_handle *handle);
 
 /*
     mappe les segments PT_LOAD en mémoire remplit handle->base_addr
 */
 int seg_load_mem(struct dl_handle *handle);
 
 /*
     relocate the symbols by looking into the .rela.dyn section
 */
 int relocations(struct dl_handle *handle);
 
 #endif /* ELF_PARSER_H */
 
 