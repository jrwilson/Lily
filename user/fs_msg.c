#include "fs_msg.h"

#include <dymem.h>
#include <string.h>

fs_descend_request_t*
fs_descend_request_create (fs_nodeid_t nodeid,
			   const char* path_begin,
			   const char* path_end)
{
  if (path_end == 0) {
    path_end = path_begin + strlen (path_begin);
  }

  fs_descend_request_t* req = malloc (sizeof (fs_descend_request_t));
  memset (req, 0, sizeof (fs_descend_request_t));
  req->nodeid = nodeid;
  size_t size = path_end - path_begin;
  req->path_begin = malloc (size);
  memcpy (req->path_begin, path_begin, size);
  req->path_end = req->path_begin + size;

  return req;
}

int
fs_descend_request_write (buffer_file_t* bf,
			  const fs_descend_request_t* req)
{
  size_t size = req->path_end - req->path_begin;
  if (buffer_file_write (bf, &req->nodeid, sizeof (fs_nodeid_t)) != 0 ||
      buffer_file_write (bf, &size, sizeof (size_t)) != 0 ||
      buffer_file_write (bf, req->path_begin, size) != 0) {
    return -1;
  }

  return 0;
}

fs_descend_request_t*
fs_descend_request_read (buffer_file_t* bf)
{
  fs_nodeid_t nodeid;
  size_t size;
  const char* path_begin;

  if (buffer_file_read (bf, &nodeid, sizeof (fs_nodeid_t)) != 0 ||
      buffer_file_read (bf, &size, sizeof (size_t)) != 0) {
    return 0;
  }

  path_begin = buffer_file_readp (bf, size);
  if (path_begin == 0) {
    return 0;
  }

  return fs_descend_request_create (nodeid, path_begin, path_begin + size);
}

void
fs_descend_request_destroy (fs_descend_request_t* req)
{
  free (req->path_begin);
  free (req);
}

void
fs_descend_response_init (fs_descend_response_t* res,
			  fs_descend_error_t error,
			  fs_nodeid_t nodeid)
{
  res->error = error;
  res->nodeid = nodeid;
}

fs_readfile_request_t*
fs_readfile_request_create (fs_nodeid_t nodeid,
			   const char* path_begin,
			   const char* path_end)
{
  if (path_end == 0) {
    path_end = path_begin + strlen (path_begin);
  }

  fs_readfile_request_t* req = malloc (sizeof (fs_readfile_request_t));
  memset (req, 0, sizeof (fs_readfile_request_t));
  req->nodeid = nodeid;
  size_t size = path_end - path_begin;
  req->path_begin = malloc (size);
  memcpy (req->path_begin, path_begin, size);
  req->path_end = req->path_begin + size;

  return req;
}

int
fs_readfile_request_write (buffer_file_t* bf,
			  const fs_readfile_request_t* req)
{
  size_t size = req->path_end - req->path_begin;
  if (buffer_file_write (bf, &req->nodeid, sizeof (fs_nodeid_t)) != 0 ||
      buffer_file_write (bf, &size, sizeof (size_t)) != 0 ||
      buffer_file_write (bf, req->path_begin, size) != 0) {
    return -1;
  }

  return 0;
}

fs_readfile_request_t*
fs_readfile_request_read (buffer_file_t* bf)
{
  fs_nodeid_t nodeid;
  size_t size;
  const char* path_begin;

  if (buffer_file_read (bf, &nodeid, sizeof (fs_nodeid_t)) != 0 ||
      buffer_file_read (bf, &size, sizeof (size_t)) != 0) {
    return 0;
  }

  path_begin = buffer_file_readp (bf, size);
  if (path_begin == 0) {
    return 0;
  }

  return fs_readfile_request_create (nodeid, path_begin, path_begin + size);
}

void
fs_readfile_request_destroy (fs_readfile_request_t* req)
{
  free (req->path_begin);
  free (req);
}

void
fs_readfile_response_init (fs_readfile_response_t* res,
			   fs_readfile_error_t error,
			   size_t size)
{
  res->error = error;
  res->size = size;
}
