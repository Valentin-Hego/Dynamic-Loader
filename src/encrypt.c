#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define XOR_KEY 0x42

int main(int argc, char **argv) {
    if (argc != 3) {
        return 1;
    }

    FILE *in = fopen(argv[1], "rb");
    FILE *out = fopen(argv[2], "wb");
    if (!in || !out) {
        if (in) fclose(in);
        return 1;
    }

    //exact size of the entry file
    fseek(in, 0, SEEK_END);
    uint64_t size = ftell(in);
    rewind(in);

    uint8_t *buffer = malloc(size);
    if (!buffer) return 1;
    
    //encryption is very simple, just a XOR
    fread(buffer, 1, size, in);
    for (uint64_t i = 0; i < size; i++) {
        buffer[i] ^= XOR_KEY;
    }
    fwrite(buffer, 1, size, out);

    // add the size of the file at the end (8 octets)
    fwrite(&size, sizeof(uint64_t), 1, out);

    free(buffer);
    fclose(in);
    fclose(out);
    
    printf("file %s encrypted, stored in %s\n", argv[1], argv[2]);
    return 0;
}