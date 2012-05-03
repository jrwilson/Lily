#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <buffer_file.h>
#include "system.h"
#include "fs_msg.h"
#include "bind_stat.h"

typedef void (*readfile_callback_t) (void* arg, fs_error_t error, size_t size, bd_t bd);

typedef struct descend_request descend_request_t;
typedef struct readfile_request readfile_request_t;
typedef struct fs fs_t;
typedef struct redirect redirect_t;
typedef struct {
  system_t* system;
  bind_stat_t* bs;
  buffer_file_t* output_bfa;
  ano_t descend_request;
  ano_t descend_response;
  descend_request_t* descend_request_head;
  descend_request_t** descend_request_tail;
  descend_request_t* descend_response_head;
  descend_request_t** descend_response_tail;
  ano_t readfile_request;
  ano_t readfile_response;
  readfile_request_t* readfile_head;
  readfile_request_t** readfile_tail;
  fs_t* fs_head;
  redirect_t* redirect_head;
  redirect_t** redirect_tail;
} vfs_t;

void
vfs_init (vfs_t* vfs,
	  system_t* system,
	  bind_stat_t* bs,
	  buffer_file_t* output_bfa,
	  ano_t descend_request,
	  ano_t descend_response,
	  ano_t readfile_request,
	  ano_t readfile_response);

void
vfs_append (vfs_t* vfs,
	    aid_t from_aid,
	    fs_nodeid_t from_nodeid,
	    aid_t to_aid,
	    fs_nodeid_t to_nodeid);

void
vfs_readfile (vfs_t* vfs,
	      const char* path_begin,
	      const char* path_end,
	      readfile_callback_t callback,
	      void* arg);

void
vfs_schedule (const vfs_t* vfs);

void
vfs_descend_request (vfs_t* vfs,
		     aid_t aid);

void
vfs_descend_response (vfs_t* vfs,
		      aid_t aid,
		      bd_t bda,
		      bd_t bdb);

#endif /* VFS_H */
