#include <stddef.h>

//__attribute__ -> not necessary meeting correction

// hidden = pas dans .dynsym = relocation RELATIVE (correction chall5)
//__attribute__((visibility("hidden"))) // can be donne in the compilation
const char *foo_exported(void)
{
    return "Hello from foo_exported";
}

//__attribute__((visibility("hidden")))
const char *bar_exported(void)
{
    return "Hello from bar_exported";
}

struct my_symbol { // custom dynamic symbols table (-> will replace .dynsym)
    const char *name;
    void       *addr;
};


struct my_symbol my_symbols[] = {
    {"foo_exported", (void *)foo_exported},
    {"bar_exported", (void *)bar_exported},
    {NULL, NULL}
};