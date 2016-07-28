#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <stdio.h>

int main() {
  int fd;
  const char* myfifo = "/tmp/myfifo";
  char buf[1];
  int rval;
  timeval tv;
  fd_set rfds;

  mkfifo(myfifo, 0666);

  fd = open(myfifo, O_RDONLY);
  if (fd < 0) {
    perror("open()");
  }
  
  while (1) {
    printf("waiting on select()\n");
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    rval = select(fd+1, &rfds, NULL, NULL, NULL);
    printf("select returned: %i\n", rval);
    if (rval < 0) {
      printf("error on select()\n");
      perror("select(): " );
    }

    rval = read(fd, buf, 1);
    printf("rval: %i\n", rval);
    if (rval < 0) {
      printf("error on read()\n");
      perror("read(): " );
    }
    else if (rval == 0) {
      printf("no bytes read\n");
      perror("read(): " );
    }
    else if (rval > 0) {
      printf("readback (%i bytes): %c\n", rval, buf[0]);
    }
  }
  close(fd);

  unlink(myfifo);
  return 0;
}
