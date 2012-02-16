#ifndef TMPFS_H
#define TMPFS_H

#include <stddef.h>

/* TODO:  Most if not all of this should come from VFS. */

/* Result codes. */
typedef enum {
  TMPFS_SUCCESS,
  TMPFS_NO_BUFFER,
  TMPFS_BUFFER_TOO_BIG,
  TMPFS_BAD_REQUEST,
  TMPFS_BAD_INODE,
} tmpfs_error_t;

typedef enum {
  TMPFS_READ_FILE,
} tmpfs_control_t;

typedef struct {
  tmpfs_control_t control;
  union {
    struct {
      size_t inode;
    } read_file;
  } u;
} tmpfs_control_request_t;

typedef struct {
  tmpfs_error_t error;
} tmpfs_control_response_t;

#endif /* TMPFS_H */
