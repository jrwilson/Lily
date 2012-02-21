#ifndef VFS_PRIV_H
#define VFS_PRIV_H

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

#endif /* VFS_PRIV_H */
