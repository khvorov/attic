#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#define SHM_FILE "/apc.shm.kashyap"

void lg(const char *oper, int result) {
  printf("%s %d\n", oper, result);
  if (result < 0) {
    perror(oper);
  }
}

void child(char *result) {
  for (int i = 0; i < 50; ++i) {
    strcpy(result, "child ::: hello parent\n");
    usleep(2);
    printf("child ::: %s", result);
  }
  usleep(5);
}

void parent(char *result) {
  usleep(1);
  for (int i = 0; i < 50; ++i) {
    strcpy(result, "parent ::: hello child\n");
    usleep(2);
    printf("parent ::: %s", result);
  }
  usleep(5);
}

int main() {
  int integerSize = 1024 * 1024 * 256; //256 mb
  
  int descriptor = -1; 
  int mmap_flags = MAP_SHARED;

#ifdef SHM
  // Open the shared memory.
  descriptor = shm_open(SHM_FILE, 
      O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

  // Size up the shared memory.
  ftruncate(descriptor, integerSize);
#else
  //descriptor = -1;
  descriptor = creat("/dev/zero", S_IRUSR | S_IWUSR);
  mmap_flags |= MAP_ANONYMOUS;
#endif
  char *result = mmap(NULL, integerSize, 
      PROT_WRITE | PROT_READ, mmap_flags, 
      descriptor, 0 );

  perror("mmap");
  printf("%X\n", result);

  pid_t child_pid = fork();

  switch(child_pid) {
    case 0:
      child(result);
      break;
    case -1:
      lg("fork", -1);
      break;
    default:
      parent(result);
  }

  lg("msync", msync(result, integerSize, MS_SYNC));
  lg("munmap", munmap(result, integerSize));

  usleep(500000);

#ifdef SHM
  if (child_pid != 0) {
    lg("shm_unlink", shm_unlink(SHM_FILE));
  }
#endif

  return 0;
}
