#include "fs_msg.h"

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
fs_descend_request_write (buffer_file_t* bf,
			  fs_nodeid_t nodeid,
			  const char* begin,
			  size_t size)
{
  fs_type_t type = FS_DESCEND;
  if (buffer_file_write (bf, &type, sizeof (fs_type_t)) != 0 ||
      buffer_file_write (bf, &nodeid, sizeof (fs_nodeid_t)) != 0 ||
      buffer_file_write (bf, &size, sizeof (size_t)) != 0 ||
      buffer_file_write (bf, begin, size) != 0) {
    return -1;
  }

  return 0;
}

int
fs_descend_request_read (buffer_file_t* bf,
			 fs_nodeid_t* nodeid,
			 const char** begin,
			 size_t* size)
{
  if (buffer_file_read (bf, nodeid, sizeof (fs_nodeid_t)) != 0 ||
      buffer_file_read (bf, size, sizeof (size_t)) != 0) {
    return -1;
  }

  *begin = buffer_file_readp (bf, *size);
  if (*begin == 0) {
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
