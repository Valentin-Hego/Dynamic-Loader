#include <stdio.h>

//les fonctions de la lib
extern const char *foo_exported(void);
extern const char *bar_exported(void);
extern const char *foo_imported(void);
extern const char *bar_imported(void);

// Définie ici, utilisée par la lib
const char *external_func(void) {
    return "Hello from external_func in main!";
}

int main(void) {
    printf("foo_exported : %s\n", foo_exported());
    printf("bar_exported : %s\n", bar_exported());
    printf("foo_imported : %s\n", foo_imported());
    printf("bar_imported : %s\n", bar_imported());
    return 0;
}
