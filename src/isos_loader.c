#define _GNU_SOURCE
#include "my_dl.h"
#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

#define XOR_KEY 0x42

// argp options
const char *argp_program_version = "isos_loader chall 6.0 (obfuscated) + bonus 2";
const char *argp_program_bug_address = "<valentin.hego@univ-rennes.fr>";

static char doc[] = "isos_loader -- load an ELF shared library and call its functions\v"
                    "Example: ./isos_loader ./libfoo.so foo_exported bar_exported\n"
                    "If a payload is embedded, don't put the lib path: ./isos_loader foo_exported";

static char args_doc[] = "[LIBRARY] FUNC [FUNC...]";

static struct argp_option options[] = {
    { 0 }
};

struct arguments {
    const char *library;
    char **functions;
    int nfunctions;
    int has_encrypted; // 1 -> encrypted is detected in the executable
    int has_lib; // 1 -> lib explicitly given
};

/* 
Search for an encrypted payload appended to the executable (AT THE END)
if found -> decrypt it in memory and write into memfd and fill path. 
*/
static int extract_embedded_payload(char *path, size_t len)
{
    FILE *f = fopen("/proc/self/exe", "rb");
    if (!f) return 0;

    fseek(f, -8, SEEK_END); // the last 8 bytes
    uint64_t size;
    fread(&size, 8, 1, f);

    if (size == 0) {
        fclose(f);
        return 0;
    }

    fseek(f, -(size + 8), SEEK_END); // move to the beginning of the payload

    uint8_t *buf = malloc(size);
    fread(buf, 1, size, f);
    fclose(f);

    //decrypt
    for (uint64_t i = 0; i < size; i++)
        buf[i] ^= XOR_KEY;

    // create a file in RAM To put the decrypted lib into it
    int fd = memfd_create("lib", 0);
    write(fd, buf, size);
    free(buf);

    // build the path to the fd for my_dlopen 
    snprintf(path, len, "/proc/self/fd/%d", fd);
    return 1;
}



/* 
Parse arguments with argp
Fills args->library and args->functions depends on embedded payload presence
correction : both mode ok -> [LIB FUNC...] or [FUNC...] 
*/
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *args = state->input; // state = structure provided by argp during parsing, it contains all parsing context
    switch (key) {
    case ARGP_KEY_ARG:
        if (args->has_encrypted) { // payload present, first arg is explicit lib (path ending with .so
            if (state->arg_num == 0 && (strstr(arg, ".so") )) {
                args->library = arg;   // override embedded payload
                args->has_lib = 1;
            } else if (state->arg_num == 0) { //first arg is function
                args->functions = &state->argv[state->next - 1]; // state field: full array of all arguments
                args->nfunctions = state->argc - (int)(state->next - 1);
                state->next = state->argc;
            } else if (state->arg_num == 1 && args->has_lib) {
                // explicit lib already read, functions start here
                args->functions = &state->argv[state->next - 1];
                args->nfunctions = state->argc - (int)(state->next - 1);
                state->next = state->argc;
            }
        } else { // no payload, classic behavior -> arg[0]=lib, arg[1..n]=functions
            if (state->arg_num == 0) { // state field: current positional argument index
                args->library = arg;
            } else if (state->arg_num == 1) {
                // points to the beginning of the function list and consumes all remaining arguments at once
                args->functions = &state->argv[state->next - 1];
                args->nfunctions = state->argc - (int)(state->next - 1);
                state->next = state->argc;
            }
        }
        break;
    case ARGP_KEY_END:
        // check that we have at least one function (payload) or lib+function (no payload)
        if (args->has_encrypted && !args->has_lib && state->arg_num < 1)
            argp_usage(state);
        else if ((!args->has_encrypted || args->has_lib) && state->arg_num < 2) // 2 because we expect -> ./isos_loader LIBRARY FUNC
            argp_usage(state);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };



int main(int argc, char *argv[])
{
    struct arguments args = { 0 };
    char memfd_path[256];// buffer that store the path to the lib stored in memory

    //detect embedded payload
    args.has_encrypted = extract_embedded_payload(memfd_path, sizeof(memfd_path));

    argp_parse(&argp, argc, argv, 0, 0, &args);

    //if payload is present and no lib in the commande override it (correction -> now it's not in both cases)
    if (args.has_encrypted && !args.has_lib) {
        args.library = memfd_path;
        printf("MODE BONUS 2 encrypted Library found and stored from the RAM.\n");
    }

    void *handle = my_dlopen(args.library);
    if (!handle) {
        fprintf(stderr, "my_dlopen -> FAIL\n");
        return EXIT_FAILURE;
    }


    for (int i = 0; i < args.nfunctions; i++) {
        const char *fname = args.functions[i];

        void *sym = my_dlsym(handle, fname); // memory address of the function in the .so
        if (!sym) {
            fprintf(stderr, "symbol '%s' not found\n", fname);
            continue;
        }
        /* 
        fn becomes a pointer to the actual function and can be called like a regular function: fn()
        then direct call and display of the result 
        */
        const char *(*fn)(void) = (const char *(*)(void))sym;
        printf("[%s] => %s\n", fname, fn());
    }

    return EXIT_SUCCESS;
}