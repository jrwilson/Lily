#include "vfs_user.h"
#include "vfs_priv.h"
#include <automaton.h>
#include <string.h>
#include <buffer_file.h>

bd_t
vfs_mount_request (aid_t aid,
		   const char* path,
		   size_t* bd_size)
{
  vfs_request_t r;
  r.type = VFS_MOUNT;
  r.u.mount.aid = aid;
  r.u.mount.path_size = strlen (path) + 1;

  size_t size = size_to_pages (sizeof (vfs_request_t) + r.u.mount.path_size);
  bd_t bd = buffer_create (size);
  if (bd == -1) {
    return -1;
  }

  buffer_file_t file;
  if (buffer_file_open (&file, bd, size, true) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  if (buffer_file_write (&file, &r, sizeof (vfs_request_t)) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  if (buffer_file_write (&file, path, r.u.mount.path_size) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  if (bd_size != 0) {
    *bd_size = buffer_file_size (&file);
  }

  buffer_unmap (bd);

  return bd;
}

int
vfs_mount_response (bd_t bd,
		    size_t bd_size,
		    vfs_error_t* error)
{
  buffer_file_t file;
  if (buffer_file_open (&file, bd, bd_size, false) == -1) {
    return -1;
  }

  const vfs_response_t* response = buffer_file_readp (&file, sizeof (vfs_response_t));
  if (response == 0) {
    return -1;
  }
  
  if (response->type != VFS_MOUNT) {
    return -1;
  }

  *error = response->error;
  return 0;
}
