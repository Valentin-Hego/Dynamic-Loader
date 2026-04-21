#include <stddef.h>

// hidden = pas dans .dynsym = relocation RELATIVE
__attribute__((visibility("hidden")))
const char *foo_exported(void)
{
    return "Hello from foo_exported";
}

__attribute__((visibility("hidden")))
const char *bar_exported(void)
{
    return "Hello from bar_exported";
}

struct my_symbol { // custom dynamic symbols table
    const char *name;
    void       *addr;
};

/* my_symbols doit rester visible pour que e_entry pointe dessus
 __attribute__ pour donner des instruction au compilateur :
 used -> ne pas supprimer cette variable même si est inutilisée
 visibility("default") -> rendre le symbole visible dans .dynsym (sinon hidden l'efface)
*/
__attribute__((used, visibility("default")))
struct my_symbol my_symbols[] = {
    {"foo_exported", (void *)foo_exported},
    {"bar_exported", (void *)bar_exported},
    {NULL, NULL}
};