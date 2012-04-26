#ifndef FS_MSG_H
#define FS_MSG_H

#include <stddef.h>
#include <buffer_file.h>

/* User section. */

/* Action names. */
#define FS_REQUEST_NAME "request"
#define FS_RESPONSE_NAME "response"

/* Identifies a node in a file system. */
typedef size_t fs_nodeid_t;

/* Errors. */
typedef enum {
  FS_SUCCESS,
  FS_BAD_REQUEST,
  FS_NODE_DNE,
  FS_NOT_DIRECTORY,
  FS_NOT_FILE,
  FS_CHILD_DNE,
} fs_error_t;

typedef void (*readfile_callback_t) (void* arg, fs_error_t error, size_t size, bd_t bd);

typedef struct fs fs_t;
typedef struct redirect redirect_t;
typedef struct {
  ano_t request;
  ano_t response;
  fs_t* fs_head;
  redirect_t* redirect_head;
  redirect_t** redirect_tail;
} vfs_t;

void
vfs_init (vfs_t* vfs,
	  ano_t request,
	  ano_t response);

int
vfs_append (vfs_t* vfs,
	    aid_t from_aid,
	    fs_nodeid_t from_nodeid,
	    aid_t to_aid,
	    fs_nodeid_t to_nodeid);

void
vfs_readfile (vfs_t* vfs,
	      const char* path,
	      readfile_callback_t callback,
	      void* arg);

void
vfs_schedule (const vfs_t* vfs);

void
vfs_request (vfs_t* vfs,
	     aid_t aid);

void
vfs_response (vfs_t* vfs,
	      aid_t aid,
	      bd_t bda,
	      bd_t bdb);

/* Implementer section. */

/* Request/response types. */
typedef enum {
  FS_DESCEND,
  FS_READFILE,
} fs_type_t;

typedef enum {
  FILE,
  DIRECTORY,
} fs_node_type_t;

int
fs_request_type_read (buffer_file_t* bf,
		      fs_type_t* type);

int
fs_descend_request_write (buffer_file_t* bf,
			  fs_nodeid_t nodeid,
			  const char* begin,
			  size_t size);

int
fs_descend_request_read (buffer_file_t* bf,
			 fs_nodeid_t* nodeid,
			 const char** begin,
			 size_t* size);

int
fs_descend_response_write (buffer_file_t* bf,
			   fs_error_t error,
			   fs_nodeid_t nodeid);

int
fs_descend_response_read (buffer_file_t* bf,
			  fs_error_t* error,
			  fs_nodeid_t* nodeid);

int
fs_readfile_request_write (buffer_file_t* bf,
			   fs_nodeid_t nodeid);

int
fs_readfile_request_read (buffer_file_t* bf,
			  fs_nodeid_t* nodeid);

int
fs_readfile_response_write (buffer_file_t* bf,
			    fs_error_t error,
			    size_t size);

int
fs_readfile_response_read (buffer_file_t* bf,
			   fs_error_t* error,
			   size_t* size);

#endif /* FS_MSG_H */
