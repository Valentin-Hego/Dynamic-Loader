#include "my_dl.h"
#include <argp.h>
#include <stdio.h>
#include <stdlib.h>


//options découvertes d'argp
const char *argp_program_version = "isos_loader chall 1.0";
const char *argp_program_bug_address = "<valentin.hego@univ-rennes.fr.com>";

static char doc[] = "isos_loader -- load an ELF shared library and call its functions\v"
                    "Example: ./isos_loader ./libfoo.so foo_exported bar_exported";

static char args_doc[] = "LIBRARY FUNC [FUNC...]";

static struct argp_option options[] = {
    { 0 }
};

struct arguments {
    const char *library;
    char **functions;
    int nfunctions;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *args = state->input;

    switch (key) {
    case ARGP_KEY_ARG:
        if (state->arg_num == 0) {
            args->library = arg;
        } else if (state->arg_num == 1) {
            // pointe sur le debut de la liste des fonctions et consomme tout le reste des arguments d'un coup
            args->functions = &state->argv[state->next - 1];
            args->nfunctions = state->argc - (int)(state->next - 1);
            state->next = state->argc;
        }
        break;

    case ARGP_KEY_END:
        if (state->arg_num < 2)
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
    argp_parse(&argp, argc, argv, 0, 0, &args);

    void *handle = my_dlopen(args.library);
    if (!handle)
        return EXIT_FAILURE;

    for (int i = 0; i < args.nfunctions; i++) {
        const char *fname = args.functions[i];

        void *sym = my_dlsym(handle, fname);
        if (!sym) {
            fprintf(stderr, "symbol '%s' not found\n", fname);
            continue;
        }

        const char *(*fn)(void) = (const char *(*)(void))sym;
        printf("[%s] => %s\n", fname, fn());
    }

    return EXIT_SUCCESS;
}
