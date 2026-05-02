#include "elf_parser.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>

/*
    Reads the ELF header from the file and performs the chall_2 checks :
 
    checks :
    1: The file starts with the ELF magic bytes (\x7fELF)
    2: The binary is 64-bit (EI_CLASS == ELFCLASS64)
    3: The binary is a shared object / dynamic library (e_type == ET_DYN)
    4: The size of our Elf64_Ehdr structure matches the e_ehsize field
    5: The number of program headers (e_phnum) is strictly greater than 0
*/
int elf_open_and_check_ehdr(const char *path, struct dl_handle *handle){

    handle->fd = open(path, O_RDONLY);
    if (handle->fd < 0) {
        fprintf(stderr, "%s: cannot open '%s'\n", __func__, path);
        return -1;
    }

    // Read exactly sizeof(Elf64_Ehdr) bytes into the structure
    ssize_t n = read(handle->fd, &handle->ehdr, sizeof(handle->ehdr));
    if (n < 0 || (size_t)n < sizeof(handle->ehdr)) {
        fprintf(stderr, "%s: failed to read ELF header from '%s'\n", __func__, path);
        goto err_close;
    }

    // check 1
    if (memcmp(handle->ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "%s: '%s' is not an ELF file\n", __func__, path);
        goto err_close;
    }

    // check 2
    if (handle->ehdr.e_ident[EI_CLASS] != ELFCLASS64) {
        fprintf(stderr, "%s: '%s' is not a 64-bit ELF\n", __func__, path);
        goto err_close;
    }

    // check 3
    if (handle->ehdr.e_type != ET_DYN) {
        fprintf(stderr, "%s: '%s' is not a dynamic library (ET_DYN)\n", __func__, path);
        goto err_close;
    }

    // check 4
    if (sizeof(handle->ehdr) != handle->ehdr.e_ehsize) {
        fprintf(stderr,
                "%s: ehdr size mismatch in '%s' "
                "(expected %u, got %zu)\n",
                __func__, path, handle->ehdr.e_ehsize, sizeof(handle->ehdr));
        goto err_close;
    }

    // check 5
    if (handle->ehdr.e_phnum == 0) {
        fprintf(stderr, "%s: '%s' has no program headers\n", __func__, path);
        goto err_close;
    }

    // everything went well
    fprintf(stderr, "%s: '%s' success " "(%u segment found)\n", __func__, path, handle->ehdr.e_phnum);
    return 0;

    err_close: // to fix a warning in CI
        close(handle->fd);
        handle->fd = -1;
        return -1;
}



/*
    Challenge 3 : find PT_LOAD segments and perform the 4 checks.
 
    Steps :
      1. Verify that sizeof(Elf64_Phdr) == handle->ehdr.e_phentsize
      2. Read all program headers from the file
      3. Filter PT_LOAD into a dynamic array
      4. Check 1 : at least one PT_LOAD
      5. Check 2 : the first PT_LOAD covers the program headers area (p_offset <= e_phoff && p_offset + p_filesz >= e_phoff + phnum * phentsize)
      6. Check 3 : ascending order of p_vaddr
      7. Check 4 : no overlap (vaddr[i] + memsz[i] <= vaddr[i+1])
      8. Compute mem_size = last.p_vaddr + last.p_memsz - first.p_vaddr
*/
int elf_find_load_segments(struct dl_handle *handle){
    Elf64_Ehdr *ehdr = &handle->ehdr;

    // Make sure that the size of your phdr is equal to the field phentsize in the ehdr.
    if (sizeof(Elf64_Phdr) != ehdr->e_phentsize){
        fprintf(stderr, "phdr size problem\n");
        return -1;
    }


    // allocation of an array to read all program headers from the file
    int nb_segments = ehdr->e_phnum;
    int size_phdr = sizeof(Elf64_Phdr);
    int total_size = nb_segments * size_phdr;
 
    Elf64_Phdr *all_phdrs = malloc(total_size);
    if (all_phdrs == NULL) {
        fprintf(stderr, "%s: failed allocation\n", __func__);
        return -1;
    }


    // move the cursor to the correct position :
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

    // iterate through all segments and keep only PT_LOAD
    Elf64_Phdr *load_segs = malloc(nb_segments * size_phdr);
    if (load_segs == NULL){
        fprintf(stderr, "%s: failed allocation\n", __func__);
        free(all_phdrs);
        return -1;
    }

    size_t load_count = 0; // number of PT_LOAD found
    
    for (int i = 0; i < nb_segments; i++) {
        if (all_phdrs[i].p_type == PT_LOAD) {
            load_segs[load_count] = all_phdrs[i]; // segment copy
            load_count++;
        }
    }
    free(all_phdrs);
    
    
    // check : The DL library has at least one load segment.
    if(load_count == 0){
        fprintf(stderr, "%s: no PT_LOAD segment found\n", __func__);
        free(load_segs);
        return -1;
    }

    // check : The first load segment spans over all segment headers.
    // p_offset <= e_phoff  AND  p_offset + p_filesz >= e_phoff + (phnum * phentsize)
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


    // check : The PT_LOAD segments are in ascending order of p_vaddr. 
    for (size_t i = 1; i < load_count; i++) {
        if (load_segs[i].p_vaddr <= load_segs[i - 1].p_vaddr) {
            fprintf(stderr, "%s: PT_LOAD segments are not in ascending order of p_vaddr\n", __func__);
            free(load_segs);
            return -1;
        }
    }    


    // check : The PT_LOAD segments do not overlap.
    for (size_t i = 0; i + 1 < load_count; i++) {
        uint64_t end_i = load_segs[i].p_vaddr + load_segs[i].p_memsz;
        if (end_i > load_segs[i + 1].p_vaddr) {
            fprintf(stderr, "%s: two PT_LOAD segments overlap in memory\n", __func__);            
            free(load_segs);
            return -1;
        }
    }


    // Compute the total memory size between the first PT_LOAD and the end of the last PT_LOAD
    uint64_t mem_size = (load_segs[load_count - 1].p_vaddr + load_segs[load_count - 1].p_memsz) - load_segs[0].p_vaddr;

    // results -> stored in the handle
    handle->load_segs  = load_segs;
    handle->load_count = load_count;
    handle->mem_size   = mem_size;

    fprintf(stderr, "%s: found %zu PT_LOAD segments, total mem size = 0x%lx\n", __func__, load_count, (unsigned long)mem_size);
    return 0;

}



/*
    Loads PT_LOAD segments of an ELF into memory.
    We first reserve the entire range to guarantee contiguity,
    then we "overwrite" regions with file mappings.
*/
 int seg_load_mem(struct dl_handle *h) {

    if (h == NULL || h->load_segs == NULL || h->load_count == 0){
        return -1;
    }
    
    // page size -> usually = 4096
    size_t psz = (size_t)sysconf(_SC_PAGESIZE);
    // round up to the next page multiple
    // mmap requires full pages and aligned pages
    size_t total = (h->mem_size + psz - 1) & ~(psz - 1);


    // initial reservation: allocate everything at once
    void *map = mmap((void *)0x00, total, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED) {
        perror("mmap_reserve");
        return -1;
    }
 
    // difference between ELF virtual address and real address
    h->base_addr = (char *)map - h->load_segs[0].p_vaddr;
    fprintf(stderr, "%s: reserved %zu bytes at %p, base_addr=%p\n", __func__, total, map, h->base_addr);
 
    // loop 1 : map each segment from the file
    for (size_t i = 0; i < h->load_count; i++) {
        Elf64_Phdr *s = &h->load_segs[i];
 
        // page alignment
        uintptr_t align_off = s->p_vaddr % psz; // possible excess 
        void *addr = (char *)h->base_addr + (s->p_vaddr - align_off); // move address backward 
        size_t len = s->p_filesz + align_off;
        off_t offset = (off_t)(s->p_offset - align_off);
 
        if (mmap(addr, len, PROT_WRITE, MAP_PRIVATE | MAP_FIXED, h->fd, offset) == MAP_FAILED) {
            perror("mmap_segment");
            goto err_cleanup;
        }
        fprintf(stderr, "%s: segment %zu mapped at %p (filesz=0x%lx, memsz=0x%lx)\n", __func__, i, (char *)h->base_addr + s->p_vaddr, (unsigned long)s->p_filesz, (unsigned long)s->p_memsz);
 
        // clean up BSS area
        if (s->p_memsz > s->p_filesz) {
            void *bss_ptr = (char *)h->base_addr + s->p_vaddr + s->p_filesz; // where BSS starts
            memset(bss_ptr, 0, s->p_memsz - s->p_filesz); // zero-fill to avoid garbage
            fprintf(stderr, "%s: BSS zeroed at %p (%zu bytes)\n", __func__, bss_ptr, s->p_memsz - s->p_filesz);
        }
    }
 
    // loop 2 : apply real permissions with mprotect
    // length based on p_memsz to cover BSS
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




/*
    Challenge 5 : Dynamic relocations
    The goal is to fix addresses in the loaded binary since it is not
    at its original base address defined at compile time.
*/
int relocations(struct dl_handle *handle) {
    Elf64_Dyn *dyn_table = NULL;
    Elf64_Rela *rela_table = NULL;
    size_t rela_size = 0;
    size_t rela_ent_size = 0;

    // find the dynamic segment
    Elf64_Phdr *phdrs = (Elf64_Phdr *)((char *)handle->base_addr + handle->ehdr.e_phoff);
    for (int i = 0; i < handle->ehdr.e_phnum; i++) {
        if (phdrs[i].p_type == PT_DYNAMIC) {
            dyn_table = (Elf64_Dyn *)((char *)handle->base_addr + phdrs[i].p_vaddr);
            break;
        }
    }

    // if no global vars or dependencies (nothing to do)
    if (dyn_table == NULL)
        return 0;

    // search RELA entries in the dynamic table
    for (int i = 0; dyn_table[i].d_tag != DT_NULL; i++) {
        if (dyn_table[i].d_tag == DT_RELA)
            rela_table = (Elf64_Rela *)((char *)handle->base_addr + dyn_table[i].d_un.d_ptr);
        else if (dyn_table[i].d_tag == DT_RELASZ)
            rela_size = dyn_table[i].d_un.d_val;
        else if (dyn_table[i].d_tag == DT_RELAENT)
            rela_ent_size = dyn_table[i].d_un.d_val;
    }

    if (rela_table == NULL || rela_ent_size == 0)
        return 0;

    int num_relocs = (int)(rela_size / rela_ent_size);
    long psz = sysconf(_SC_PAGESIZE);

    // loop over all entries
    for (int i = 0; i < num_relocs; i++) {
        Elf64_Rela *curr = (Elf64_Rela *)((char *)rela_table + (i * rela_ent_size));
        uint32_t type = ELF64_R_TYPE(curr->r_info); // in subject, rela can be R_X86_64_RELATIVE or R_X86_64_64

        void *target_addr = (char *)handle->base_addr + curr->r_offset; // address to fix
        uint64_t final_val = 0;

        if (type == R_X86_64_RELATIVE || type == R_X86_64_64) {
            // address = base + addend 
            final_val = (uint64_t)handle->base_addr + (uint64_t)curr->r_addend; // correction
        }

        // temporary mprotect if the page is not writable
        int modif_prot = 0;
        int orig_prot  = PROT_READ | PROT_WRITE;

        for (size_t j = 0; j < handle->load_count; j++) {
            Elf64_Phdr *seg = &handle->load_segs[j];
            void *seg_start = (char *)handle->base_addr + seg->p_vaddr;
            void *seg_end   = (char *)seg_start + seg->p_memsz;

            if (target_addr >= seg_start && target_addr < seg_end) {
                // reconstruct original prot
                orig_prot = ((seg->p_flags & PF_R) ? PROT_READ  : 0) |
                            ((seg->p_flags & PF_W) ? PROT_WRITE : 0) |
                            ((seg->p_flags & PF_X) ? PROT_EXEC  : 0);
                // mprotect only if segment is not already writable            
                if (!(seg->p_flags & PF_W))
                    modif_prot = 1;
                break;
            }
        }

        // same principle as chall 4: mprotect works per page
        uintptr_t page_start = (uintptr_t)target_addr & ~((uintptr_t)psz - 1);
        if (mprotect((void *)page_start, (size_t)psz, PROT_READ | PROT_WRITE) < 0) {
            perror("mprotect (reloc start)");
            return -1;
        }
        
        // apply relocation
        *(uint64_t *)target_addr = final_val;

        // restore original permissions if the segment was read-only
        if (modif_prot) {
            if (mprotect((void *)page_start, (size_t)psz, orig_prot) < 0) {
                perror("mprotect (reloc restore)");
                return -1;
            }
        }
    }

    fprintf(stderr, "relocations: Applied %d relocations\n", num_relocs);
    return 0;
}