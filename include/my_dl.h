#ifndef MY_DL_H
#define MY_DL_H

/*
  my_dlopen() that loads a DL library named by its string argument, and returns an
  opaque “handle” (void *) for the loaded object.
*/
void *my_dlopen(const char *path);

/*
  my_dlsym() that takes a “handle” of a loaded object along with a string function name,
  and returns the address where that function is loaded into memory.
*/
void *my_dlsym(void *handle, const char *name);

#endif /* MY_DL_H */
