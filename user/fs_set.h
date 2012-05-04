#ifndef FS_SET_H
#define FS_SET_H

#include <stddef.h>
#include <buffer_file.h>
#include "system.h"
#include "fs_msg.h"
#include "bind_stat.h"

typedef void (*readfile_callback_t) (void* arg, const fs_readfile_response_t* res, bd_t bd);

typedef struct fs fs_t;
typedef struct {
  system_t* system;
  bind_stat_t* bs;
  buffer_file_t* output_bfa;
  ano_t descend_request;
  ano_t descend_response;
  ano_t readfile_request;
  ano_t readfile_response;
  fs_t* fs_head;
  fs_t* active_fs;
} fs_set_t;

void
fs_set_init (fs_set_t* fs_set,
	     system_t* system,
	     bind_stat_t* bs,
	     buffer_file_t* output_bfa,
	     ano_t descend_request,
	     ano_t descend_response,
	     ano_t readfile_request,
	     ano_t readfile_response);

void
fs_set_insert (fs_set_t* fs_set,
	       aid_t aid,
	       const char* name,
	       fs_nodeid_t nodeid);

void
fs_set_make_active (fs_set_t* fs_set,
		    aid_t aid);

void
fs_set_readfile (fs_set_t* fs_set,
		 const char* path_begin,
		 const char* path_end,
		 readfile_callback_t callback,
		 void* arg);

void
fs_set_descend_request (fs_set_t* fs_set,
			aid_t aid);

void
fs_set_descend_response (fs_set_t* fs_set,
			 aid_t aid,
			 bd_t bda,
			 bd_t bdb);

void
fs_set_readfile_request (fs_set_t* fs_set,
			 aid_t aid);

void
fs_set_readfile_response (fs_set_t* fs_set,
			  aid_t aid,
			  bd_t bda,
			  bd_t bdb);

void
fs_set_schedule (const fs_set_t* fs_set);

#endif /* FS_SET_H */
