#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  char buffer[256];
  int fd;
  bool err = false;
  ssize_t read_bytes, written_bytes;

  if (argc == 1) {  // If no arguments, read from standard input
    while ((read_bytes = read(STDIN_FILENO, buffer, sizeof(buffer) - 1)) > 0) {
      written_bytes = write(STDOUT_FILENO, buffer, read_bytes);
      if (written_bytes != read_bytes) {
        perror("bobcat: write error");
        exit(1);
      }
    }
  } else {
    for (int i = 1; i < argc; ++i) {
      if (argv[i][0] == '-' &&
          argv[i][1] == '\0') {  // Check if argument is "-"
        while ((read_bytes = read(STDIN_FILENO, buffer, sizeof(buffer) - 1)) >
               0) {
          written_bytes = write(STDOUT_FILENO, buffer, read_bytes);
          if (written_bytes != read_bytes) {
            perror("bobcat: write error");
            exit(1);
          }
        }
      } else {
        fd = open(argv[i], O_RDONLY);
        if (fd == -1) {
          err = true;
          fprintf(stderr, "bobcat: %s: No such file or directory\n", argv[i]);
          continue;
        } else {
          while ((read_bytes = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
            written_bytes = write(STDOUT_FILENO, buffer, read_bytes);
            if (written_bytes != read_bytes) {
              perror("bobcat: write error");
              close(fd);
              exit(1);
            }
          }
          close(fd);
        }
      }
    }
  }

  if (err) {
    exit(1);
  }
  exit(0);
}
