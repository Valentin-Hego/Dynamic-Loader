#include "my_dl.h"
#include <dlfcn.h>
#include <stdio.h>

void *my_dlopen(const char *path)
{
    if (!path) {
        fprintf(stderr, "my_dlopen: path is null\n");
        return NULL;
    }

    void *handle = dlopen(path, RTLD_LAZY);
    if (!handle)
        fprintf(stderr, "my_dlopen: %s\n", dlerror());

    return handle;
}

void *my_dlsym(void *handle, const char *name)
{
    if (!handle || !name) {
        fprintf(stderr, "my_dlsym: invalid arguments\n");
        return NULL;
    }

    dlerror(); // si erreurs precedentes on les vides
    void *sym = dlsym(handle, name);

    const char *err = dlerror();
    if (err){
        fprintf(stderr, "my_dlsym: %s\n", err);
	return NULL;
    }
    return sym;
}
