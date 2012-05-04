#ifndef FS_MSG_H
#define FS_MSG_H

#include <stddef.h>
#include <buffer_file.h>

/* File system clients look for the following names which means file system servers should name their actions appropriately. */
#define FS_DESCEND_REQUEST_IN_NAME "fs_descend_request_in"
#define FS_DESCEND_RESPONSE_OUT_NAME "fs_descend_response_out"
#define FS_READFILE_REQUEST_IN_NAME "fs_readfile_request_in"
#define FS_READFILE_RESPONSE_OUT_NAME "fs_readfile_response_out"

/* Identifies a node in a file system. */
typedef size_t fs_nodeid_t;

/* The type of a node.  Other options include hard links and symbolic links. */
typedef enum {
  FILE,
  DIRECTORY,
} fs_node_type_t;

typedef struct {
  fs_nodeid_t nodeid;
  char* name_begin;
  char* name_end;
} fs_descend_request_t;

fs_descend_request_t*
fs_descend_request_create (fs_nodeid_t nodeid,
			   const char* name_begin,
			   const char* name_end);

int
fs_descend_request_write (buffer_file_t* bfa,
			  const fs_descend_request_t* req);

fs_descend_request_t*
fs_descend_request_read (buffer_file_t* bfa);

void
fs_descend_request_destroy (fs_descend_request_t* req);

typedef enum {
  FS_DESCEND_SUCCESS,
  FS_DESCEND_NODE_DNE,
  FS_DESCEND_NOT_DIRECTORY,
  FS_DESCEND_CHILD_DNE,
} fs_descend_error_t;

typedef struct {
  fs_descend_error_t error;
  fs_nodeid_t nodeid;
} fs_descend_response_t;

void
fs_descend_response_init (fs_descend_response_t* res,
			  fs_descend_error_t error,
			  fs_nodeid_t nodeid);

typedef struct {
  fs_nodeid_t nodeid;
} fs_readfile_request_t;

void
fs_readfile_request_init (fs_readfile_request_t* res,
			  fs_nodeid_t nodeid);

typedef enum {
  FS_READFILE_SUCCESS,
  FS_READFILE_NODE_DNE,
  FS_READFILE_NOT_FILE,
} fs_readfile_error_t;

typedef struct {
  fs_readfile_error_t error;
  size_t size;
} fs_readfile_response_t;

void
fs_readfile_response_init (fs_readfile_response_t* res,
			   fs_readfile_error_t error,
			   size_t size);

#endif /* FS_MSG_H */
