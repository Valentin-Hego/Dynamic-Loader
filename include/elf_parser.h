#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <elf.h>
#include <stdint.h>

/*
    structure pour stocker de handle retourné par my_dlopen()
*/
struct dl_handle {
    int         fd;     // file descriptor du .so
    Elf64_Ehdr  ehdr;   // copie du header de l'executable
};


/*
    ouvre la shared library au path, lis son ELF header into handle->ehdr
    et fait les vérifs du chall_2
 
    Returns : 0 -> success, -1 -> error (mess d'erreurs sur le stderr)
 */
int elf_open_and_check_ehdr(const char *path, struct dl_handle *handle);

#endif /* ELF_PARSER_H */
