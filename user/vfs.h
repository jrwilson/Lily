#ifndef VFS_H
#define VFS_H

#include <stddef.h>

/* User section. */

#define VFS_REQUEST_NAME "request"
#define VFS_RESPONSE_NAME "response"

typedef enum {
  VFS_SUCCESS,
  VFS_NO_BUFFER,
  VFS_NO_MAP,
  VFS_BAD_REQUEST,
  VFS_BAD_PATH,
} vfs_error_t;

typedef enum {
  VFS_UNKNOWN,
  VFS_MOUNT,
} vfs_type_t;

typedef struct {
  vfs_type_t type;
  union {
    struct {
      aid_t aid;	/* The automaton to mount. */
      size_t path_size;	/* The size of the path including the null terminator.  The path follows the request. */
    } mount;
  } u;
} vfs_request_t;

typedef struct {
  vfs_type_t type;
  vfs_error_t error;
} vfs_response_t;

/* File system section. */

#define VFS_FS_REQUEST_NAME "vfs_request"
#define VFS_FS_RESPONSE_NAME "vfs_response"

/* Result codes. */
typedef enum {
  VFS_FS_SUCCESS,
  VFS_FS_NO_BUFFER,
  VFS_FS_NO_MAP,
  VFS_FS_BAD_REQUEST,
  VFS_FS_BAD_INODE,
} vfs_fs_error_t;

typedef enum {
  VFS_FS_UNKNOWN,
  VFS_FS_READ_FILE,
} vfs_fs_type_t;

/*
  A request buffer is structured as follows:
  +------------------------------+
  | vfs_fs_request_t (mandatory) |
  +------------------------------+ (Page boundary)
  | data (optional)              |
  +------------------------------+

  A response buffer is structured the same way.
 */

typedef struct {
  vfs_fs_type_t type;
  union {
    struct {
      size_t inode;
    } read_file;
  } u;
} vfs_fs_request_t;

typedef struct {
  vfs_fs_type_t type;
  vfs_fs_error_t error;
  union {
    struct {
      size_t size;
    } read_file;
  } u;
} vfs_fs_response_t;

#endif /* VFS_H */
