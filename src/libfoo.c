// déclaration de fonction qui est définie dans le prog appelant
extern const char *external_func(void);

const char *foo_exported(void) {
    return "Hello from foo_exported";
}

const char *bar_exported(void) {
    return "Hello from bar_exported";
}


const char *foo_imported(void) {
    return external_func();
}

const char *bar_imported(void) {
    return external_func();
}
