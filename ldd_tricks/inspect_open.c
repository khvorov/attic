#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>

typedef int (*orig_open_ftype)(const char *, int);

int open(const char * path, int flags, ...)
{
    orig_open_ftype orig_open;
    orig_open = (orig_open_ftype) dlsym(RTLD_NEXT, "open");

    printf("The victim used open(...) to access '%s'!!!\n", path);
    
    return orig_open(path, flags);
}
