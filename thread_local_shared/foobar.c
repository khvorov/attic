// gcc -Wall -g -shared -o libfoobar.so -fPIC foobar.c

#include <stdio.h>

int g_foobar = 0;

int plus_foobar (void)
{
    return g_foobar++;
}

int print_foobar (char *prefix)
{
    return printf("[print_foobar] %s: (%p) %d\n", prefix, &g_foobar, g_foobar);
}
