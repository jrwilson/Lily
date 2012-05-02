#ifndef FS_MSG_H
#define FS_MSG_H

#include <stddef.h>
#include <buffer_file.h>

/* Action names. */
#define FS_REQUEST_NAME "fs_request_in"
#define FS_RESPONSE_NAME "fs_response_out"

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
