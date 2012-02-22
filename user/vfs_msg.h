#ifndef VFS_USER_H
#define VFS_USER_H

#include <stddef.h>
#include <lily/types.h>

/*
  User Section
  ============
*/

/* Action names. */
#define VFS_REQUEST_NAME "request"
#define VFS_RESPONSE_NAME "response"

/* Request types. */
typedef enum {
  VFS_UNKNOWN,
  VFS_MOUNT,
} vfs_type_t;

/* Result codes. */
typedef enum {
  VFS_SUCCESS,
  VFS_BAD_REQUEST,
  VFS_BAD_REQUEST_TYPE,
  VFS_BAD_PATH,
} vfs_error_t;

int
read_vfs_request_type (bd_t bd,
		       size_t bd_size,
		       vfs_type_t* type);

bd_t
write_vfs_response (vfs_type_t type,
		    vfs_error_t error,
		    size_t* bd_size);

int
read_vfs_response (bd_t bd,
		   size_t bd_size,
		   vfs_type_t* type,
		   vfs_error_t* error);

bd_t
write_vfs_mount_request (aid_t aid,
			 const char* path,
			 size_t* bd_size);

int
read_vfs_mount_request (bd_t bd,
			size_t bd_size,
			aid_t* aid,
			const char** path,
			size_t* path_size);

/*
  File System Section
  ===================
*/

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

#endif /* VFS_USER_H */
