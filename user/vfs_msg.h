#ifndef VFS_USER_H
#define VFS_USER_H

#include <stddef.h>
#include <lily/types.h>

#define VFS_DESCRIPTION "vfs"
#define VFS_DESCRIPTION_SIZE 4

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
  VFS_READFILE,
} vfs_type_t;

/* Result codes. */
typedef enum {
  VFS_SUCCESS,
  VFS_BAD_REQUEST,
  VFS_BAD_REQUEST_TYPE,
  VFS_BAD_PATH,
  VFS_PATH_DNE,
  VFS_NOT_DIRECTORY,
  VFS_NOT_FILE,
  VFS_AID_DNE,
  VFS_NOT_FS,
  VFS_ALREADY_MOUNTED,
  VFS_NOT_AVAILABLE,
} vfs_error_t;

int
read_vfs_request_type (bd_t bd,
		       vfs_type_t* type);

bd_t
write_vfs_unknown_response (vfs_error_t error);

bd_t
write_vfs_mount_request (aid_t aid,
			 const char* path);

int
read_vfs_mount_request (bd_t bd,
			aid_t* aid,
			const char** path,
			size_t* path_size);

bd_t
write_vfs_mount_response (vfs_error_t error);

int
read_vfs_mount_response (bd_t bd,
			 vfs_error_t* error);

bd_t
write_vfs_readfile_request (const char* path);

int
read_vfs_readfile_request (bd_t bd,
			   const char** path,
			   size_t* path_size);

bd_t
write_vfs_readfile_response (vfs_error_t error,
			     size_t size);

int
read_vfs_readfile_response (bd_t bd,
			    vfs_error_t* error,
			    size_t* size);

/*
  File System Section
  ===================
*/

#define VFS_FS_REQUEST_NAME "vfs_request"
#define VFS_FS_RESPONSE_NAME "vfs_response"

typedef enum {
  FILE,
  DIRECTORY,
} vfs_fs_node_type_t;

typedef struct {
  vfs_fs_node_type_t type;
  size_t id;
} vfs_fs_node_t;

/* Request types. */
typedef enum {
  VFS_FS_UNKNOWN,
  VFS_FS_DESCEND,
  VFS_FS_READFILE,
} vfs_fs_type_t;

/* Result codes. */
typedef enum {
  VFS_FS_SUCCESS,
  VFS_FS_BAD_REQUEST,
  VFS_FS_BAD_REQUEST_TYPE,
  VFS_FS_BAD_NODE,
  VFS_FS_NOT_DIRECTORY,
  VFS_FS_NOT_FILE,
  VFS_FS_CHILD_DNE,
} vfs_fs_error_t;

int
read_vfs_fs_request_type (bd_t bd,
			  vfs_fs_type_t* type);

bd_t
write_vfs_fs_unknown_response (vfs_fs_error_t error);

bd_t
write_vfs_fs_descend_request (size_t id,
			      const char* name,
			      size_t name_size);

int
read_vfs_fs_descend_request (bd_t bd,
			     size_t* id,
			     const char** name,
			     size_t* name_size);

bd_t
write_vfs_fs_descend_response (vfs_fs_error_t error,
			       const vfs_fs_node_t* node);

int
read_vfs_fs_descend_response (bd_t bd,
			      vfs_fs_error_t* error,
			      vfs_fs_node_t* node);

bd_t
write_vfs_fs_readfile_request (size_t id);

int
read_vfs_fs_readfile_request (bd_t bd,
			      size_t* id);

bd_t
write_vfs_fs_readfile_response (vfs_fs_error_t error,
				size_t size);

int
read_vfs_fs_readfile_response (bd_t bd,
			       vfs_fs_error_t* error,
			       size_t* size);

#endif /* VFS_USER_H */
