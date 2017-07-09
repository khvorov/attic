#define _GNU_SOURCE

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <errno.h>

#define FOOBAR_LIBRARY "/home/sammy/work/thread_local_shared/libfoobar.so"
#undef _COPY_SO_LIB

static int hardlinking = 0;

typedef int (*inc_fn)(void);
typedef int (*prn_fn)(char *);

void inc_your_copy (int howmuch, inc_fn f)
{
    while (howmuch--) f();
}

void print_your_copy (char *id, prn_fn f)
{
    char self[64];
    snprintf(self, 64, "%8s (%lu)", id, pthread_self());
    f(self);
}

void *thread_fn (void *arg)
{
    inc_fn i;
    prn_fn p;
    char *id = arg;
    int *foobar;
    void *plugin_handle;

#ifdef _COPY_SO_LIB
    char libname[256] = "/tmp/87XXXXXX";
    char copy_command[256];
    int fd;

    /* make a copy of the library */
    if (!hardlinking) {
        fd = mkstemp(libname);
        close(fd);
        snprintf(copy_command, 256, "cp %s %s", FOOBAR_LIBRARY, libname);
        system(copy_command);
    } else {
        mktemp(libname);
        fd = link(FOOBAR_LIBRARY, libname);
        if (fd != 0) {
            perror(libname);
            return NULL;
        }
    }

    /* open the copy */
    plugin_handle = dlopen(libname, RTLD_LOCAL|RTLD_LAZY);
#else
    const char * libname = FOOBAR_LIBRARY;
    plugin_handle = dlmopen(LM_ID_NEWLM, libname, RTLD_NOW);
#endif

    if (NULL == plugin_handle) return NULL;

    /* lookup the global symbols */
    i           = dlsym(plugin_handle, "plus_foobar");
    p           = dlsym(plugin_handle, "print_foobar");
    foobar      = dlsym(plugin_handle, "g_foobar");

    if (!i || !p || !foobar)
        return NULL;

    /* some threads directly access global variable and
     * some threads do it using a function pointer */
    if (*id & 1)
        inc_your_copy(100, i);
    else
        *foobar += 10;

    /* print twice; once yourself, once by function pointer */
    printf("[thread] %8s (%lu): (%p) %d\n", id, pthread_self(), foobar, *foobar);
    print_your_copy(id, p);

    /* clean up the mess */
    sleep(1); // just to ensure everyone loads their copy
    dlclose(plugin_handle); // before dlclose() for mapping

#ifdef _COPY_SO_LIB
    unlink(libname);
#endif
}

int main (int ac, char **av)
{
    pthread_t threads[ac];

    do {
        pthread_create(&threads[--ac], NULL, thread_fn, av[ac-1]);
        usleep(10);
    } while (ac);
    pthread_join(threads[0], NULL);
    return 0;
}