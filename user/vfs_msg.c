#include "vfs_msg.h"
#include <automaton.h>
#include <string.h>
#include <buffer_file.h>
#include <dymem.h>

void
vfs_request_queue_init (vfs_request_queue_t* vrq)
{
  vrq->head = 0;
  vrq->tail = &vrq->head;
}

bool
vfs_request_queue_empty (const vfs_request_queue_t* vrq)
{
  return vrq->head == 0;
}

const vfs_request_queue_item_t*
vfs_request_queue_front (const vfs_request_queue_t* vrq)
{
  return vrq->head;
}

void
vfs_request_queue_push_mount (vfs_request_queue_t* vrq,
			      aid_t aid,
			      const char* path)
{
  vfs_request_queue_item_t* item = malloc (sizeof (vfs_request_queue_item_t));
  memset (item, 0, sizeof (vfs_request_queue_item_t));
  item->type = VFS_MOUNT;
  item->u.mount.aid = aid;
  size_t size = strlen (path) + 1;
  item->u.mount.path = malloc (size);
  memcpy (item->u.mount.path, path, size);
  item->u.mount.path_size = size;
  
  *vrq->tail = item;
  vrq->tail = &item->next;
}

void
vfs_request_queue_push_readfile (vfs_request_queue_t* vrq,
				 const char* path)
{
  vfs_request_queue_item_t* item = malloc (sizeof (vfs_request_queue_item_t));
  memset (item, 0, sizeof (vfs_request_queue_item_t));
  item->type = VFS_READFILE;
  size_t size = strlen (path) + 1;
  item->u.readfile.path = malloc (size);
  memcpy (item->u.readfile.path, path, size);
  item->u.readfile.path_size = size;
  
  *vrq->tail = item;
  vrq->tail = &item->next;
}

int
vfs_request_queue_pop_to_buffer (vfs_request_queue_t* vrq,
				 bd_t bda,
				 bd_t bdb)
{
  vfs_request_queue_item_t* item = vrq->head;

  buffer_file_t file;
  if (buffer_file_initc (&file, bda) == -1) {
    return -1;
  }

  if (item->type == VFS_UNKNOWN) {
    return -1;
  }

  if (buffer_file_write (&file, &item->type, sizeof (vfs_type_t)) == -1) {
    return -1;
  }

  switch (item->type) {
  case VFS_UNKNOWN:
    return -1;
    break;
  case VFS_MOUNT:
    if (buffer_file_write (&file, &item->u.mount.aid, sizeof (aid_t)) == -1 ||
	buffer_file_write (&file, &item->u.mount.path_size, sizeof (size_t)) == -1 ||
	buffer_file_write (&file, item->u.mount.path, item->u.mount.path_size) == -1) {
      return -1;
    }
    break;
  case VFS_READFILE:
    if (buffer_file_write (&file, &item->u.readfile.path_size, sizeof (size_t)) == -1 ||
	buffer_file_write (&file, item->u.readfile.path, item->u.readfile.path_size) == -1) {
      return -1;
    }
    break;
  }

  vfs_request_queue_pop (vrq);

  return 0;
}

int
vfs_request_queue_push_from_buffer (vfs_request_queue_t* vrq,
				    bd_t bda,
				    bd_t bdb,
				    vfs_type_t* type)
{
  *type = VFS_UNKNOWN;

  buffer_file_t file;
  if (buffer_file_initr (&file, bda) == -1) {
    return -1;
  }

  if (buffer_file_read (&file, type, sizeof (vfs_type_t)) == -1) {
    return -1;
  }

  switch (*type) {
  case VFS_UNKNOWN:
    return -1;
    break;
  case VFS_MOUNT:
    {
      aid_t aid;
      size_t path_size;
      const char* path;
      if (buffer_file_read (&file, &aid, sizeof (aid_t)) == -1 ||
	  buffer_file_read (&file, &path_size, sizeof (size_t)) == -1) {
	return -1;
      }

      path = buffer_file_readp (&file, path_size);
      if (path == 0) {
	return -1;
      }
      if (path[path_size - 1] != 0) {
	return -1;
      }

      vfs_request_queue_push_mount (vrq, aid, path);
    }
    break;
  case VFS_READFILE:
    {
      size_t path_size;
      const char* path;
      if (buffer_file_read (&file, &path_size, sizeof (size_t)) == -1) {
	return -1;
      }

      path = buffer_file_readp (&file, path_size);
      if (path == 0) {
	return -1;
      }
      if (path[path_size - 1] != 0) {
	return -1;
      }

      vfs_request_queue_push_readfile (vrq, path);
    }
    break;
  default:
    *type = VFS_UNKNOWN;
    return -1;
  }

  return 0;
}

void
vfs_request_queue_pop (vfs_request_queue_t* vrq)
{
  vfs_request_queue_item_t* item = vrq->head;

  switch (item->type) {
  case VFS_UNKNOWN:
    break;
  case VFS_MOUNT:
    free (item->u.mount.path);
    break;
  case VFS_READFILE:
    free (item->u.readfile.path);
    break;
  }

  vrq->head = item->next;
  if (vrq->head == 0) {
    vrq->tail = &vrq->head;
  }

  free (item);
}

struct vfs_response_queue_item {
  vfs_type_t type;
  vfs_error_t error;
  union {
    struct {
      size_t size;
      bd_t bd;
    } readfile;
  } u;
  vfs_response_queue_item_t* next;
};

void
vfs_response_queue_init (vfs_response_queue_t* vrq)
{
  vrq->head = 0;
  vrq->tail = &vrq->head;
}

bool
vfs_response_queue_empty (const vfs_response_queue_t* vrq)
{
  return vrq->head == 0;
}

void
vfs_response_queue_push_bad_request (vfs_response_queue_t* vrq,
				     vfs_type_t type)
{
  vfs_response_queue_item_t* item = malloc (sizeof (vfs_response_queue_item_t));
  memset (item, 0, sizeof (vfs_response_queue_item_t));
  item->type = type;
  item->error = VFS_BAD_REQUEST;
  
  *vrq->tail = item;
  vrq->tail = &item->next;
}

void
vfs_response_queue_push_mount (vfs_response_queue_t* vrq,
			       vfs_error_t error)
{
  vfs_response_queue_item_t* item = malloc (sizeof (vfs_response_queue_item_t));
  memset (item, 0, sizeof (vfs_response_queue_item_t));
  item->type = VFS_MOUNT;
  item->error = error;
  
  *vrq->tail = item;
  vrq->tail = &item->next;
}

int
vfs_response_queue_push_readfile (vfs_response_queue_t* vrq,
				  vfs_error_t error,
				  size_t size,
				  bd_t bd)
{
  vfs_response_queue_item_t* item = malloc (sizeof (vfs_response_queue_item_t));
  memset (item, 0, sizeof (vfs_response_queue_item_t));
  item->type = VFS_READFILE;
  item->error = error;
  item->u.readfile.size = size;
  if (bd != -1) {
    item->u.readfile.bd = buffer_copy (bd);
    if (item->u.readfile.bd == -1) {
      return -1;
    }
  }
  else {
    item->u.readfile.bd = -1;
  }
  
  *vrq->tail = item;
  vrq->tail = &item->next;

  return 0;
}

int
vfs_response_queue_pop_to_buffer (vfs_response_queue_t* vrq,
				  bd_t bda,
				  bd_t bdb)
{
  vfs_response_queue_item_t* item = vrq->head;

  buffer_file_t file;
  if (buffer_file_initc (&file, bda) == -1) {
    return -1;
  }

  if (item->type == VFS_UNKNOWN) {
    return -1;
  }

  if (buffer_file_write (&file, &item->type, sizeof (vfs_type_t)) == -1 ||
      buffer_file_write (&file, &item->error, sizeof (vfs_error_t)) == -1) {
    return -1;
  }

  switch (item->type) {
  case VFS_UNKNOWN:
    return -1;
    break;
  case VFS_MOUNT:
    /* Nothing. */
    break;
  case VFS_READFILE:
    if (buffer_file_write (&file, &item->u.readfile.size, sizeof (size_t)) == -1) {
      return -1;
    }
    if (item->u.readfile.bd != -1) {
      if (buffer_assign (bdb, item->u.readfile.bd) == -1) {
	return -1;
      }
      buffer_destroy (item->u.readfile.bd);
    }
    else {
      if (buffer_resize (bdb, 0) == -1) {
	return -1;
      }
    }
    break;
  }

  vrq->head = item->next;
  if (vrq->head == 0) {
    vrq->tail = &vrq->head;
  }

  free (item);

  return 0;
}

int
read_vfs_mount_response (bd_t bda,
			 bd_t bdb,
			 vfs_error_t* error)
{
  buffer_file_t file;
  if (buffer_file_initr (&file, bda) == -1) {
    return -1;
  }
  
  vfs_type_t type;
  if (buffer_file_read (&file, &type, sizeof (vfs_type_t)) == -1 ||
      type != VFS_MOUNT ||
      buffer_file_read (&file, error, sizeof (vfs_error_t)) == -1) {
    return -1;
  }

  return 0;
}

int
read_vfs_readfile_response (bd_t bd,
			    vfs_error_t* error,
			    size_t* size)
{
  buffer_file_t file;
  if (buffer_file_initr (&file, bd) == -1) {
    return -1;
  }
  
  vfs_type_t type;
  if (buffer_file_read (&file, &type, sizeof (vfs_type_t)) == -1 ||
      type != VFS_READFILE ||
      buffer_file_read (&file, error, sizeof (vfs_error_t)) == -1 ||
      buffer_file_read (&file, size, sizeof (size_t)) == -1) {
    return -1;
  }

  return 0;
}

/* FILE SYSTEM SECTION. */

int
read_vfs_fs_request_type (bd_t bda,
			  bd_t bdb,
			  vfs_fs_type_t* type)
{
  buffer_file_t file;
  if (buffer_file_initr (&file, bda) == -1) {
    return -1;
  }

  if (buffer_file_read (&file, type, sizeof (vfs_fs_type_t)) == -1) {
    return -1;
  }

  return 0;
}

int
write_vfs_fs_unknown_response (vfs_fs_error_t error,
			       bd_t* bda,
			       bd_t* bdb)
{
  size_t bd_sz = size_to_pages (sizeof (vfs_fs_type_t) + sizeof (vfs_fs_error_t));
  bd_t bd = buffer_create (bd_sz);
  if (bd == -1) {
    return -1;
  }

  buffer_file_t file;
  if (buffer_file_initc (&file, bd) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  const vfs_fs_type_t type = VFS_FS_UNKNOWN;
  if (buffer_file_write (&file, &type, sizeof (vfs_type_t)) == -1 ||
      buffer_file_write (&file, &error, sizeof (vfs_error_t)) == -1) {
    buffer_destroy (bd);
    return -1;
  }
  
  *bda = bd;
  *bdb = -1;

  return 0;
}

int
read_vfs_fs_descend_request (bd_t bda,
			     bd_t bdb,
			     size_t* id,
			     const char** name,
			     size_t* name_size)
{
  buffer_file_t file;
  if (buffer_file_initr (&file, bda) == -1) {
    return -1;
  }

  vfs_fs_type_t type;
  if (buffer_file_read (&file, &type, sizeof (vfs_fs_type_t)) == -1 ||
      type != VFS_FS_DESCEND ||
      buffer_file_read (&file, id, sizeof (size_t)) == -1 ||
      buffer_file_read (&file, name_size, sizeof (size_t)) == -1) {
    return -1;
  }
  
  const char* p = buffer_file_readp (&file, *name_size);
  if (p == 0) {
    return -1;
  }

  *name = p;

  return 0;
}

int
write_vfs_fs_descend_response (vfs_fs_error_t error,
			       const vfs_fs_node_t* node,
			       bd_t* bda,
			       bd_t* bdb)
{
  size_t bd_sz = size_to_pages (sizeof (vfs_fs_type_t) + sizeof (vfs_fs_error_t) + sizeof (vfs_fs_node_t));
  bd_t bd = buffer_create (bd_sz);
  if (bd == -1) {
    return -1;
  }

  buffer_file_t file;
  if (buffer_file_initc (&file, bd) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  const vfs_fs_type_t type = VFS_FS_DESCEND;
  if (buffer_file_write (&file, &type, sizeof (vfs_fs_type_t)) == -1 ||
      buffer_file_write (&file, &error, sizeof (vfs_fs_error_t)) == -1 ||
      buffer_file_write (&file, node, sizeof (vfs_fs_node_t)) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  *bda = bd;
  *bdb = -1;

  return 0;
}

int
read_vfs_fs_descend_response (bd_t bda,
			      bd_t bdb,
			      vfs_fs_error_t* error,
			      vfs_fs_node_t* node)
{
  buffer_file_t file;
  if (buffer_file_initr (&file, bda) == -1) {
    return -1;
  }
  
  vfs_fs_type_t type;
  if (buffer_file_read (&file, &type, sizeof (vfs_fs_type_t)) == -1 ||
      type != VFS_FS_DESCEND ||
      buffer_file_read (&file, error, sizeof (vfs_fs_error_t)) == -1 ||
      buffer_file_read (&file, node, sizeof (vfs_fs_node_t)) == -1) {
    return -1;
  }

  return 0;
}

int
read_vfs_fs_readfile_request (bd_t bda,
			      bd_t bdb,
			      size_t* id)
{
  buffer_file_t file;
  if (buffer_file_initr (&file, bda) == -1) {
    return -1;
  }
  
  vfs_fs_type_t type;
  if (buffer_file_read (&file, &type, sizeof (vfs_fs_type_t)) == -1 ||
      type != VFS_FS_READFILE ||
      buffer_file_read (&file, id, sizeof (size_t)) == -1) {
    return -1;
  }

  return 0;
}

int
write_vfs_fs_readfile_response (vfs_fs_error_t error,
				size_t size,
				bd_t* bda)
{
  size_t bd_sz = size_to_pages (sizeof (vfs_fs_type_t) + sizeof (vfs_fs_error_t) + sizeof (size_t));
  bd_t bd = buffer_create (bd_sz);
  if (bd == -1) {
    return -1;
  }

  buffer_file_t file;
  if (buffer_file_initc (&file, bd) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  const vfs_fs_type_t type = VFS_FS_READFILE;
  if (buffer_file_write (&file, &type, sizeof (vfs_fs_type_t)) == -1 ||
      buffer_file_write (&file, &error, sizeof (vfs_fs_error_t)) == -1 ||
      buffer_file_write (&file, &size, sizeof (size_t)) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  *bda = bd;

  return 0;
}

int
read_vfs_fs_readfile_response (bd_t bda,
			       bd_t bdb,
			       vfs_fs_error_t* error,
			       size_t* size)
{
  buffer_file_t file;
  if (buffer_file_initr (&file, bda) == -1) {
    return -1;
  }
  
  vfs_fs_type_t type;
  if (buffer_file_read (&file, &type, sizeof (vfs_fs_type_t)) == -1 ||
      type != VFS_FS_READFILE ||
      buffer_file_read (&file, error, sizeof (vfs_fs_error_t)) == -1 ||
      buffer_file_read (&file, size, sizeof (size_t)) == -1) {
    return -1;
  }

  return 0;

}

void
vfs_fs_request_queue_init (vfs_fs_request_queue_t* vrq)
{
  vrq->head = 0;
  vrq->tail = &vrq->head;
}

bool
vfs_fs_request_queue_empty (const vfs_fs_request_queue_t* vrq)
{
  return vrq->head == 0;
}

void
vfs_fs_request_queue_push_descend (vfs_fs_request_queue_t* vrq,
				   size_t id,
				   const char* name,
				   size_t name_size)
{
  vfs_fs_request_queue_item_t* item = malloc (sizeof (vfs_fs_request_queue_item_t));
  memset (item, 0, sizeof (vfs_fs_request_queue_item_t));
  item->type = VFS_FS_DESCEND;
  item->u.descend.id = id;
  item->u.descend.name = malloc (name_size);
  memcpy (item->u.descend.name, name, name_size);
  item->u.descend.name_size = name_size;
  
  *vrq->tail = item;
  vrq->tail = &item->next;
}

void
vfs_fs_request_queue_push_readfile (vfs_fs_request_queue_t* vrq,
				    size_t id)
{
  vfs_fs_request_queue_item_t* item = malloc (sizeof (vfs_fs_request_queue_item_t));
  memset (item, 0, sizeof (vfs_fs_request_queue_item_t));
  item->type = VFS_FS_READFILE;
  item->u.readfile.id = id;
  
  *vrq->tail = item;
  vrq->tail = &item->next;
}

int
vfs_fs_request_queue_pop_to_buffer (vfs_fs_request_queue_t* vrq,
				    bd_t bda,
				    bd_t bdb)
{
  vfs_fs_request_queue_item_t* item = vrq->head;

  buffer_file_t file;
  if (buffer_file_initc (&file, bda) == -1) {
    return -1;
  }

  if (item->type == VFS_FS_UNKNOWN) {
    return -1;
  }

  if (buffer_file_write (&file, &item->type, sizeof (vfs_fs_type_t)) == -1) {
    return -1;
  }

  switch (item->type) {
  case VFS_FS_UNKNOWN:
    return -1;
    break;
  case VFS_FS_DESCEND:
    if (buffer_file_write (&file, &item->u.descend.id, sizeof (size_t)) == -1 ||
	buffer_file_write (&file, &item->u.descend.name_size, sizeof (size_t)) == -1 ||
	buffer_file_write (&file, item->u.descend.name, item->u.descend.name_size) == -1) {
      return -1;
    }
    break;
  case VFS_FS_READFILE:
    if (buffer_file_write (&file, &item->u.readfile.id, sizeof (size_t)) == -1) {
      return -1;
    }
    break;
  }

  vfs_fs_request_queue_pop (vrq);

  return 0;
}

void
vfs_fs_request_queue_pop (vfs_fs_request_queue_t* vrq)
{
  vfs_fs_request_queue_item_t* item = vrq->head;

  switch (item->type) {
  case VFS_FS_UNKNOWN:
    break;
  case VFS_FS_DESCEND:
    free (item->u.descend.name);
    break;
  case VFS_FS_READFILE:
    break;
  }

  vrq->head = item->next;
  if (vrq->head == 0) {
    vrq->tail = &vrq->head;
  }

  free (item);
}

struct vfs_fs_response_queue_item {
  vfs_fs_type_t type;
  vfs_fs_error_t error;
  union {
    struct {
      vfs_fs_node_t node;
    } descend;
    struct {
      size_t size;
      bd_t bd;
    } readfile;
  } u;
  vfs_fs_response_queue_item_t* next;
};

void
vfs_fs_response_queue_init (vfs_fs_response_queue_t* vrq)
{
  vrq->head = 0;
  vrq->tail = &vrq->head;
}

bool
vfs_fs_response_queue_empty (const vfs_fs_response_queue_t* vrq)
{
  return vrq->head == 0;
}

void
vfs_fs_response_queue_push_bad_request (vfs_fs_response_queue_t* vrq,
					vfs_fs_type_t type)
{
  vfs_fs_response_queue_item_t* item = malloc (sizeof (vfs_fs_response_queue_item_t));
  memset (item, 0, sizeof (vfs_fs_response_queue_item_t));
  item->type = type;
  item->error = VFS_FS_BAD_REQUEST;
  
  *vrq->tail = item;
  vrq->tail = &item->next;
}

void
vfs_fs_response_queue_push_descend (vfs_fs_response_queue_t* vrq,
				    vfs_fs_error_t error,
				    const vfs_fs_node_t* node)
{
  vfs_fs_response_queue_item_t* item = malloc (sizeof (vfs_fs_response_queue_item_t));
  memset (item, 0, sizeof (vfs_fs_response_queue_item_t));
  item->type = VFS_FS_DESCEND;
  item->error = error;
  memcpy (&item->u.descend.node, node, sizeof (vfs_fs_node_t));
  
  *vrq->tail = item;
  vrq->tail = &item->next;
}

int
vfs_fs_response_queue_push_readfile (vfs_fs_response_queue_t* vrq,
				     vfs_fs_error_t error,
				     size_t size,
				     bd_t bd)
{
  vfs_fs_response_queue_item_t* item = malloc (sizeof (vfs_fs_response_queue_item_t));
  memset (item, 0, sizeof (vfs_fs_response_queue_item_t));
  item->type = VFS_FS_READFILE;
  item->error = error;
  item->u.readfile.size = size;
  if (bd != -1) {
    item->u.readfile.bd = buffer_copy (bd);
    if (item->u.readfile.bd == -1) {
      return -1;
    }
  }
  else {
    item->u.readfile.bd = -1;
  }
  
  *vrq->tail = item;
  vrq->tail = &item->next;

  return 0;
}

int
vfs_fs_response_queue_pop_to_buffer (vfs_fs_response_queue_t* vrq,
				     bd_t bda,
				     bd_t bdb)
{
  vfs_fs_response_queue_item_t* item = vrq->head;

  buffer_file_t file;
  if (buffer_file_initc (&file, bda) == -1) {
    return -1;
  }

  if (item->type == VFS_FS_UNKNOWN) {
    return -1;
  }

  if (buffer_file_write (&file, &item->type, sizeof (vfs_fs_type_t)) == -1 ||
      buffer_file_write (&file, &item->error, sizeof (vfs_fs_error_t)) == -1) {
    return -1;
  }

  switch (item->type) {
  case VFS_FS_UNKNOWN:
    return -1;
    break;
  case VFS_FS_DESCEND:
    if (buffer_file_write (&file, &item->u.descend.node, sizeof (vfs_fs_node_t)) == -1) {
      return -1;
    }
    break;
  case VFS_FS_READFILE:
    if (buffer_file_write (&file, &item->u.readfile.size, sizeof (size_t)) == -1) {
      return -1;
    }
    if (buffer_assign (bdb, item->u.readfile.bd) == -1) {
      return -1;
    }
    break;
  }

  vfs_fs_response_queue_pop (vrq);

  return 0;
}

void
vfs_fs_response_queue_pop (vfs_fs_response_queue_t* vrq)
{
  vfs_fs_response_queue_item_t* item = vrq->head;

  switch (item->type) {
  case VFS_FS_UNKNOWN:
    /* Nothing. */
    break;
  case VFS_FS_DESCEND:
    /* Nothing. */
    break;
  case VFS_FS_READFILE:
    if (item->u.readfile.bd != -1) {
      buffer_destroy (item->u.readfile.bd);
    }
    break;
  }

  vrq->head = item->next;
  if (vrq->head == 0) {
    vrq->tail = &vrq->head;
  }

  free (item);
}
