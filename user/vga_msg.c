#include "vga_msg.h"

int
vga_op_list_initw (vga_op_list_t* vol,
		   bd_t bda,
		   bd_t bdb)
{
  if (buffer_file_initw (&vol->bf, bda) != 0) {
    return -1;
  }

  vol->count = 0;
  vol->bdb = bdb;

  if (buffer_file_seek (&vol->bf, sizeof (size_t)) != 0) {
    return -1;
  }

  return 0;
}

int
vga_op_list_reset (vga_op_list_t* vol)
{
  if (vol->count == 0) {
    return 0;
  }

  vol->count = 0;

  buffer_file_truncate (&vol->bf);

  if (buffer_file_seek (&vol->bf, sizeof (size_t)) != 0) {
    return -1;
  }

  if (buffer_resize (vol->bdb, 0) != 0) {
    return -1;
  }

  return 0;
}

static int
increment_count (vga_op_list_t* vol)
{
  ++vol->count;

  size_t position = buffer_file_position (&vol->bf);

  if (buffer_file_seek (&vol->bf, 0) != 0) {
    return -1;
  }
  if (buffer_file_write (&vol->bf, &vol->count, sizeof (size_t)) != 0) {
    return -1;
  }
  if (buffer_file_seek (&vol->bf, position) != 0) {
    return -1;
  }

  return 0;
}

int
vga_op_list_write_set_cursor_location (vga_op_list_t* vol,
				       size_t location)
{
  vga_op_type_t type = VGA_SET_CURSOR_LOCATION;
  if (buffer_file_write (&vol->bf, &type, sizeof (vga_op_type_t)) != 0 ||
      buffer_file_write (&vol->bf, &location, sizeof (size_t)) != 0) {
    return -1;
  }

  return increment_count (vol);
}

int
vga_op_list_write_assign (vga_op_list_t* vol,
			  size_t address,
			  const void* data,
			  size_t size)
{
  vga_op_type_t type = VGA_ASSIGN;
  if (buffer_file_write (&vol->bf, &type, sizeof (vga_op_type_t)) != 0 ||
      buffer_file_write (&vol->bf, &address, sizeof (size_t)) != 0 ||
      buffer_file_write (&vol->bf, &size, sizeof (size_t)) != 0 ||
      buffer_file_write (&vol->bf, data, size) != 0) {
    return -1;
  }

  return increment_count (vol);
}

int
vga_op_list_write_bassign (vga_op_list_t* vol,
			   size_t address,
			   size_t size,
			   bd_t bd)
{
  size_t bd_size = buffer_size (bd);
  if (bd_size == -1 || size > bd_size * pagesize ()) {
    /* The buffer was too small. */
    return -1;
  }

  /* Find the offset in pages of the data. */
  size_t bd_offset = buffer_size (vol->bdb);
  /* Append the data. */
  if (buffer_append (vol->bdb, bd) != 0) {
    return -1;
  }
  
  vga_op_type_t type = VGA_BASSIGN;
  if (buffer_file_write (&vol->bf, &type, sizeof (vga_op_type_t)) != 0 ||
      buffer_file_write (&vol->bf, &address, sizeof (size_t)) != 0 ||
      buffer_file_write (&vol->bf, &size, sizeof (size_t)) != 0 ||
      buffer_file_write (&vol->bf, &bd_offset, sizeof (size_t)) != 0) {
    return -1;
  }

  return increment_count (vol);
}

int
vga_op_list_initr (vga_op_list_t* vol,
		   bd_t bda,
		   bd_t bdb,
		   size_t* count)
{
  if (buffer_file_initr (&vol->bf, bda) != 0) {
    return -1;
  }

  if (buffer_file_read (&vol->bf, &vol->count, sizeof (size_t)) != 0) {
    return -1;
  }

  vol->bdb = bdb;
  vol->bdb_size = buffer_size (bdb);
  vol->ptr = buffer_map (bdb);

  *count = vol->count;

  return 0;
}

int
vga_op_list_next_op_type (vga_op_list_t* vol,
			  vga_op_type_t* type)
{
  /* Save the current position. */
  size_t pos = buffer_file_position (&vol->bf);

  if (buffer_file_read (&vol->bf, type, sizeof (vga_op_type_t)) != 0) {
    return -1;
  }

  /* Reset. */
  if (buffer_file_seek (&vol->bf, pos) != 0) {
    return -1;
  }

  return 0;
}

int
vga_op_list_read_set_cursor_location (vga_op_list_t* vol,
				      size_t* location)
{
  vga_op_type_t type;
  if (buffer_file_read (&vol->bf, &type, sizeof (vga_op_type_t)) != 0 ||
      type != VGA_SET_CURSOR_LOCATION ||
      buffer_file_read (&vol->bf, location, sizeof (size_t)) != 0) {
    return -1;
  }

  return 0;
}

int
vga_op_list_read_assign (vga_op_list_t* vol,
			 size_t* address,
			 const void** data,
			 size_t* size)
{
  vga_op_type_t type;
  if (buffer_file_read (&vol->bf, &type, sizeof (vga_op_type_t)) != 0 ||
      type != VGA_ASSIGN ||
      buffer_file_read (&vol->bf, address, sizeof (size_t)) != 0 ||
      buffer_file_read (&vol->bf, size, sizeof (size_t)) != 0) {
    return -1;
  }

  const void* p = buffer_file_readp (&vol->bf, *size);
  if (p == 0) {
    return -1;
  }

  *data = p;

  return 0;
}

int
vga_op_list_read_bassign (vga_op_list_t* vol,
			  size_t* address,
			  const void** data,
			  size_t* size)
{
  vga_op_type_t type;
  size_t bd_offset;
  if (buffer_file_read (&vol->bf, &type, sizeof (vga_op_type_t)) != 0 ||
      type != VGA_BASSIGN ||
      buffer_file_read (&vol->bf, address, sizeof (size_t)) != 0 ||
      buffer_file_read (&vol->bf, size, sizeof (size_t)) != 0 ||
      buffer_file_read (&vol->bf, &bd_offset, sizeof (size_t)) != 0) {
    return -1;
  }

  if (vol->ptr == 0) {
    return -1;
  }

  if (vol->bdb_size == -1 || bd_offset >= vol->bdb_size) {
    return -1;
  }

  *data = vol->ptr + bd_offset * pagesize ();

  return 0;
}
