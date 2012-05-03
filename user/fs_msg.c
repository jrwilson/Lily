#include "fs_msg.h"

#include <dymem.h>
#include <string.h>

fs_descend_request_t*
fs_descend_request_create (fs_nodeid_t nodeid,
			   const char* name_begin,
			   const char* name_end)
{
  if (name_end == 0) {
    name_end = name_begin + strlen (name_begin);
  }

  fs_descend_request_t* req = malloc (sizeof (fs_descend_request_t));
  memset (req, 0, sizeof (fs_descend_request_t));
  req->nodeid = nodeid;
  size_t size = name_end - name_begin;
  req->name_begin = malloc (size);
  memcpy (req->name_begin, name_begin, size);
  req->name_end = req->name_begin + size;

  return req;
}

int
fs_descend_request_write (buffer_file_t* bf,
			  const fs_descend_request_t* req)
{
  size_t size = req->name_end - req->name_begin;
  if (buffer_file_write (bf, &req->nodeid, sizeof (fs_nodeid_t)) != 0 ||
      buffer_file_write (bf, &size, sizeof (size_t)) != 0 ||
      buffer_file_write (bf, req->name_begin, size) != 0) {
    return -1;
  }

  return 0;
}

fs_descend_request_t*
fs_descend_request_read (buffer_file_t* bf)
{
  fs_nodeid_t nodeid;
  size_t size;
  const char* name_begin;

  if (buffer_file_read (bf, &nodeid, sizeof (fs_nodeid_t)) != 0 ||
      buffer_file_read (bf, &size, sizeof (size_t)) != 0) {
    return 0;
  }

  name_begin = buffer_file_readp (bf, size);
  if (name_begin == 0) {
    return 0;
  }

  return fs_descend_request_create (nodeid, name_begin, name_begin + size);
}

void
fs_descend_request_destroy (fs_descend_request_t* req)
{
  free (req->name_begin);
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







int
fs_request_type_read (buffer_file_t* bf,
		      fs_type_t* type)
{
  if (buffer_file_read (bf, type, sizeof (fs_type_t)) != 0) {
    return -1;
  }

  return 0;
}

int
fs_descend_response_write (buffer_file_t* bf,
			   fs_error_t error,
			   fs_nodeid_t nodeid)
{
  if (buffer_file_write (bf, &error, sizeof (fs_error_t)) != 0 ||
      buffer_file_write (bf, &nodeid, sizeof (fs_nodeid_t)) != 0) {
    return -1;
  }

  return 0;
}

int
fs_descend_response_read (buffer_file_t* bf,
			  fs_error_t* error,
			  fs_nodeid_t* nodeid)
{
  if (buffer_file_read (bf, error, sizeof (fs_error_t)) != 0 ||
      buffer_file_read (bf, nodeid, sizeof (fs_nodeid_t)) != 0) {
    return -1;
  }

  return 0;
}

int
fs_readfile_request_write (buffer_file_t* bf,
			   fs_nodeid_t nodeid)
{
  fs_type_t type = FS_READFILE;
  if (buffer_file_write (bf, &type, sizeof (fs_type_t)) != 0 ||
      buffer_file_write (bf, &nodeid, sizeof (fs_nodeid_t)) != 0) {
    return -1;
  }

  return 0;
}

int
fs_readfile_request_read (buffer_file_t* bf,
			  fs_nodeid_t* nodeid)
{
  if (buffer_file_read (bf, nodeid, sizeof (fs_nodeid_t)) != 0) {
    return -1;
  }

  return 0;
}

int
fs_readfile_response_write (buffer_file_t* bf,
			    fs_error_t error,
			    size_t size)
{
  if (buffer_file_write (bf, &error, sizeof (fs_error_t)) != 0 ||
      buffer_file_write (bf, &size, sizeof (size_t)) != 0) {
    return -1;
  }

  return 0;
}

int
fs_readfile_response_read (buffer_file_t* bf,
			   fs_error_t* error,
			   size_t* size)
{
  if (buffer_file_read (bf, error, sizeof (fs_error_t)) != 0 ||
      buffer_file_read (bf, size, sizeof (size_t)) != 0) {
    return -1;
  }

  return 0;
}
