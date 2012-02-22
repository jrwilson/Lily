#include "vfs_msg.h"
#include <automaton.h>
#include <string.h>
#include <buffer_file.h>

int
read_vfs_request_type (bd_t bd,
		       size_t bd_size,
		       vfs_type_t* type)
{
  buffer_file_t file;
  if (buffer_file_open (&file, bd, bd_size, false) == -1) {
    return -1;
  }

  if (buffer_file_read (&file, type, sizeof (vfs_type_t)) == -1) {
    return -1;
  }

  return 0;
}

bd_t
write_vfs_response (vfs_type_t type,
		    vfs_error_t error,
		    size_t* bd_size)
{
  size_t bd_sz = size_to_pages (sizeof (vfs_type_t) + sizeof (vfs_error_t));
  bd_t bd = buffer_create (bd_sz);
  if (bd == -1) {
    return -1;
  }

  buffer_file_t file;
  if (buffer_file_open (&file, bd, bd_sz, true) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  if (buffer_file_write (&file, &type, sizeof (vfs_type_t)) == -1 ||
      buffer_file_write (&file, &error, sizeof (vfs_error_t)) == -1) {
    buffer_destroy (bd);
    return -1;
  }
  
  if (bd_size != 0) {
    *bd_size = buffer_file_size (&file);
  }

  return bd;
}

int
read_vfs_response (bd_t bd,
		   size_t bd_size,
		   vfs_type_t* type,
		   vfs_error_t* error)
{
  buffer_file_t file;
  if (buffer_file_open (&file, bd, bd_size, false) == -1) {
    return -1;
  }
  
  if (buffer_file_read (&file, type, sizeof (vfs_type_t)) == -1 ||
      buffer_file_read (&file, error, sizeof (vfs_error_t)) == -1) {
    return -1;
  }

  return 0;
}

bd_t
write_vfs_mount_request (aid_t aid,
			 const char* path,
			 size_t* bd_size)
{
  vfs_type_t type = VFS_MOUNT;
  size_t path_size = strlen (path) + 1;

  size_t bd_sz = size_to_pages (sizeof (vfs_type_t) + sizeof (aid_t) + sizeof (size_t) + path_size);
  bd_t bd = buffer_create (bd_sz);
  if (bd == -1) {
    return -1;
  }

  buffer_file_t file;
  if (buffer_file_open (&file, bd, bd_sz, true) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  if (buffer_file_write (&file, &type, sizeof (vfs_type_t)) == -1 ||
      buffer_file_write (&file, &aid, sizeof (aid_t)) == -1 ||
      buffer_file_write (&file, &path_size, sizeof (size_t)) == -1 ||
      buffer_file_write (&file, path, path_size) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  if (bd_size != 0) {
    *bd_size = buffer_file_size (&file);
  }

  return bd;
}

int
read_vfs_mount_request (bd_t bd,
			size_t bd_size,
			aid_t* aid,
			const char** path,
			size_t* path_size)
{
  buffer_file_t file;
  if (buffer_file_open (&file, bd, bd_size, false) == -1) {
    return -1;
  }

  vfs_type_t type;
  if (buffer_file_read (&file, &type, sizeof (vfs_type_t)) == -1 ||
      type != VFS_MOUNT ||
      buffer_file_read (&file, aid, sizeof (aid_t)) == -1 ||
      buffer_file_read (&file, path_size, sizeof (size_t)) == -1) {
    return -1;
  }
  
  const char* p = buffer_file_readp (&file, *path_size);
  if (p == 0) {
    return -1;
  }

  *path = p;

  return 0;
}
