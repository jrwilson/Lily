#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

int
main (int argc,
      char** argv)
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s FILE\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  const char* filename = argv[1];

  struct stat sb;
  if (stat (filename, &sb) == -1) {
    fprintf (stderr, "Could not stat %s: %s\n", filename, strerror (errno));
    exit (EXIT_FAILURE);
  }

  int fd = open (filename, O_RDWR);
  if (fd == -1) {
    fprintf (stderr, "Could not open %s: %s\n", filename, strerror (errno));
    exit (EXIT_FAILURE);
  }

  size_t size = sb.st_size;
  if (write (fd, &size, sizeof (size_t)) != sizeof (size_t)) {
    fprintf (stderr, "Could not write to %s: %s\n", filename, strerror (errno));
    exit (EXIT_FAILURE);
  }

  close (fd);

  exit (EXIT_SUCCESS);
}
