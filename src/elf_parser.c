#include "elf_parser.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>

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

    handle->fd = open(path, O_RDONLY);
    if (handle->fd < 0) {
        fprintf(stderr, "%s: cannot open '%s'\n", __func__, path);
        return -1;
    }

    // Lire exactement sizeof(Elf64_Ehdr) octets dans la structure
    ssize_t n = read(handle->fd, &handle->ehdr, sizeof(handle->ehdr));
    if (n < 0 || (size_t)n < sizeof(handle->ehdr)) {
        fprintf(stderr, "%s: failed to read ELF header from '%s'\n", __func__, path);
        close(handle->fd);
        handle->fd = -1;
        return -1;
    }

    // check 1
    if (memcmp(handle->ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "%s: '%s' is not an ELF file\n", __func__, path);
        close(handle->fd);
        handle->fd = -1;
        return -1;
    }

    // check 2
    if (handle->ehdr.e_ident[EI_CLASS] != ELFCLASS64) {
        fprintf(stderr, "%s: '%s' is not a 64-bit ELF\n", __func__, path);
        close(handle->fd);
        handle->fd = -1;
        return -1;
    }

    // check 3
    if (handle->ehdr.e_type != ET_DYN) {
        fprintf(stderr, "%s: '%s' is not a dynamic library (ET_DYN)\n", __func__, path);
        close(handle->fd);
        handle->fd = -1;
        return -1;
    }

    // check 4
    if (sizeof(handle->ehdr) != handle->ehdr.e_ehsize) {
        fprintf(stderr,
                "%s: ehdr size mismatch in '%s' "
                "(expected %u, got %zu)\n",
                __func__, path, handle->ehdr.e_ehsize, sizeof(handle->ehdr));
        close(handle->fd);
        handle->fd = -1;
        return -1;
    }

    // check 5
    if (handle->ehdr.e_phnum == 0) {
        fprintf(stderr, "%s: '%s' has no program headers\n", __func__, path);
        close(handle->fd);
        handle->fd = -1;
        return -1;
    }

    // tout s'est bien passé
    fprintf(stderr,
            "%s: '%s' success "
            "(%u segment found)\n",
            __func__, path, handle->ehdr.e_phnum);
    return 0;
}



/*
    Challenge 3 : trouve les segments PT_LOAD et effectue les 4 vérifications.
 
    Étapes :
      1. Vérifier que sizeof(Elf64_Phdr) == handle->ehdr.e_phentsize
      2. Lire tous les program headers depuis le fichier
      3. Filtrer les PT_LOAD dans un tableau dynamique
      4. Check 1 : au moins un PT_LOAD
      5. Check 2 : le premier PT_LOAD couvre la zone des program headers (p_offset <= e_phoff && p_offset + p_filesz >= e_phoff + phnum * phentsize)
      6. Check 3 : ordre croissant de p_vaddr
      7. Check 4 : pas de chevauchement (vaddr[i] + memsz[i] <= vaddr[i+1])
      8. Calcule mem_size = last.p_vaddr + last.p_memsz - first.p_vaddr
*/
int elf_find_load_segments(struct dl_handle *handle){
    Elf64_Ehdr *ehdr = &handle->ehdr;

    // Make sure that the size of your phdr is equal to the field phentsize in the ehdr.
    if (sizeof(Elf64_Phdr) != ehdr->e_phentsize){
        fprintf(stderr, "phdr size problem\n");
        return -1;
    }


    //allocation d'un tableau pour lire tous les program headers du fichier
    int nb_segments = ehdr->e_phnum;
    int size_phdr = sizeof(Elf64_Phdr);
    int total_size = nb_segments * size_phdr;
 
    Elf64_Phdr *all_phdrs = malloc(total_size);
    if (all_phdrs == NULL) {
        fprintf(stderr, "%s: failed allocation\n", __func__);
        return -1;
    }


    //on place le curseur au bon endroit :
    if (lseek(handle->fd, (off_t)handle->ehdr.e_phoff, SEEK_SET) < 0) {
        fprintf(stderr, "%s: cursor positionning error\n", __func__);
        free(all_phdrs);
        return -1;
    }
 
    ssize_t octets_lus = read(handle->fd, all_phdrs, total_size);
    if (octets_lus < 0 || octets_lus < total_size) {
        fprintf(stderr, "%s: headers reading failed\n", __func__);
        free(all_phdrs);
        return -1;
    }

    //parcourir tous les segments et ne garder que les PT_LOAD
    Elf64_Phdr *load_segs = malloc(nb_segments * size_phdr);
    if (load_segs == NULL){
        fprintf(stderr, "%s: failed allocation\n", __func__);
        free(all_phdrs);
        return -1;
    }

    size_t load_count = 0; //nb de PT_LOAD trouvés
    
    for (int i = 0; i < nb_segments; i++) {
        if (all_phdrs[i].p_type == PT_LOAD) {
            load_segs[load_count] = all_phdrs[i]; // copie du segment
            load_count++;
        }
    }
    free(all_phdrs);
    
    
    //check : The DL library has at least one load segment.
    if(load_count == 0){
        fprintf(stderr, "%s: no PT_LOAD segment found\n", __func__);
        free(load_segs);
        return -1;
    }

    //check : The first load segment spans over all segment headers.
    // p_offset <= e_phoff  ET  p_offset + p_filesz >= e_phoff + (phnum * phentsize)
    uint64_t ph_table_start = ehdr->e_phoff;
    uint64_t ph_table_end   = ehdr->e_phoff + (uint64_t)ehdr->e_phnum * ehdr->e_phentsize;
    uint64_t seg0_start     = load_segs[0].p_offset;
    uint64_t seg0_end       = load_segs[0].p_offset + load_segs[0].p_filesz;
    
    if (seg0_start > ph_table_start || seg0_end < ph_table_end) {
        fprintf(stderr,
                "%s: first PT_LOAD does not span program headers table\n", __func__);
        free(load_segs);
        return -1;
    }


    //check : The PT_LOAD segments are in ascending order of p_vaddr. 
    for (size_t i = 1; i < load_count; i++) {
        if (load_segs[i].p_vaddr <= load_segs[i - 1].p_vaddr) {
            fprintf(stderr, "%s: PT_LOAD segments are not in ascending order of p_vaddr\n", __func__);
            free(load_segs);
            return -1;
        }
    }    


    //check : The PT_LOAD segments do not overlap.
    for (size_t i = 0; i + 1 < load_count; i++) {
        uint64_t end_i = load_segs[i].p_vaddr + load_segs[i].p_memsz;
        if (end_i > load_segs[i + 1].p_vaddr) {
            fprintf(stderr, "%s: deux segments PT_LOAD se chevauchent en mémoire\n", __func__);            
            free(load_segs);
            return -1;
        }
    }


    //Compute the total memory size between the first PT_LOAD and the end of the last PT_LOAD
    uint64_t mem_size = (load_segs[load_count - 1].p_vaddr + load_segs[load_count - 1].p_memsz) - load_segs[0].p_vaddr;

    //les results -> dans le handle
    handle->load_segs  = load_segs;
    handle->load_count = load_count;
    handle->mem_size   = mem_size;

    fprintf(stderr, "%s: found %zu PT_LOAD segments, total mem size = 0x%lx\n", __func__, load_count, (unsigned long)mem_size);
    return 0;

}



/*
    Charge les segments PT_LOAD d'un ELF en mémoire.
    On réserve d'abord toute la plage pour garantir la contiguïté,
    puis on "écrase" les zones avec les mappings du fichier.
*/
 int seg_load_mem(struct dl_handle *h) {

    if (h == NULL || h->load_segs == NULL || h->load_count == 0){
        return -1;
    }
    
    //taille d'une page -> en général = 4096
    size_t psz = (size_t)sysconf(_SC_PAGESIZE);
    // arrondi au multiple de page supérieur
    // mmap exige nb entiers de page et pages allignées
    // comme ça le dernier segment ne peut pas deborder sur la page
    size_t total = (h->mem_size + psz - 1) & ~(psz - 1);
 
    // Réservation initiale : on prend toute la place d'un coup
    void *map = mmap((void *)0x00, total, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED) {
        perror("mmap_reserve");
        return -1;
    }
 
    // différence entre l'adresse virtuelle ELF et l'adresse réelle
    h->base_addr = (char *)map - h->load_segs[0].p_vaddr;
    fprintf(stderr, "%s: reserved %zu bytes at %p, base_addr=%p\n", __func__, total, map, h->base_addr);
 
    // boucle 1 : mapper chaque segment depuis le fichier
    for (size_t i = 0; i < h->load_count; i++) {
        Elf64_Phdr *s = &h->load_segs[i];
 
        // alignement sur les pages
        uintptr_t align_off = s->p_vaddr % psz; //l'exces potentiel 
        void *addr = (char *)h->base_addr + (s->p_vaddr - align_off); //on recule l'addresse 
        size_t len = s->p_filesz + align_off;
        off_t offset = (off_t)(s->p_offset - align_off);
 
        if (mmap(addr, len, PROT_WRITE, MAP_PRIVATE | MAP_FIXED, h->fd, offset) == MAP_FAILED) {
            perror("mmap_segment");
            goto err_cleanup;
        }
        fprintf(stderr, "%s: segment %zu mapped at %p (filesz=0x%lx, memsz=0x%lx)\n", __func__, i, (char *)h->base_addr + s->p_vaddr, (unsigned long)s->p_filesz, (unsigned long)s->p_memsz);
 
        // nettoyage de la zone BSS (mémoire non initialisée)
        if (s->p_memsz > s->p_filesz) { //si la mem reservée est plus grande que ce que l'on a besoin
            void *bss_ptr = (char *)h->base_addr + s->p_vaddr + s->p_filesz; //là que BSS commence
            memset(bss_ptr, 0, s->p_memsz - s->p_filesz); //rempli la fin de 0 pour eviter le garbage
            fprintf(stderr, "%s: BSS zeroed at %p (%zu bytes)\n", __func__, bss_ptr, s->p_memsz - s->p_filesz);
        }
    }
 
    // boucle 2 : appliquer les vraies permissions avec mprotect
    // longueur basée sur p_memsz pour couvrir la BSS
    for (size_t i = 0; i < h->load_count; i++) {
        Elf64_Phdr *s = &h->load_segs[i];
 
        int prot = ((s->p_flags & PF_R) ? PROT_READ  : 0) |
                   ((s->p_flags & PF_W) ? PROT_WRITE : 0) |
                   ((s->p_flags & PF_X) ? PROT_EXEC  : 0);
 
        uintptr_t align_off = s->p_vaddr % psz;
        void *addr = (char *)h->base_addr + (s->p_vaddr - align_off);
        size_t len = s->p_memsz + align_off;
 
        if (mprotect(addr, len, prot) < 0) {
            perror("mprotect");
            goto err_cleanup;
        }
        fprintf(stderr, "%s: segment %zu mprotect at %p (prot=%d)\n", __func__, i, (char *)h->base_addr + s->p_vaddr, prot);
    }
 
    fprintf(stderr, "%s: library fully mapped at base_addr=%p\n", __func__, h->base_addr);
    return 0;
 
 err_cleanup:
    h->base_addr = NULL;
    munmap(map, total);
    return -1;
 }