#include "vga_msg.h"

int
vga_op_list_initw (vga_op_list_t* vol,
		   bd_t* bda,
		   bd_t* bdb)
{
  bd_t bd1 = buffer_create (1);
  if (bd1 == -1) {
    return -1;
  }

  if (buffer_file_initc (&vol->bf, bd1) == -1) {
    buffer_destroy (bd1);
    return -1;
  }

  bd_t bd2 = buffer_create (0);
  if (bd2 == -1) {
    buffer_destroy (bd1);
    return -1;
  }

  vol->count = 0;
  vol->bdb = bd2;

  if (buffer_file_write (&vol->bf, &vol->count, sizeof (size_t)) == -1) {
    buffer_destroy (bd1);
    buffer_destroy (bd2);
    return -1;
  }

  *bda = bd1;
  *bdb = bd2;

  return 0;
}

int
vga_op_list_write_set_cursor_location (vga_op_list_t* vol,
				       size_t location)
{
  vga_op_type_t type = VGA_SET_CURSOR_LOCATION;
  if (buffer_file_write (&vol->bf, &type, sizeof (vga_op_type_t)) == -1 ||
      buffer_file_write (&vol->bf, &location, sizeof (size_t)) == -1) {
    return -1;
  }

  ++vol->count;

  size_t position = buffer_file_position (&vol->bf);

  if (buffer_file_seek (&vol->bf, 0) == -1) {
    return -1;
  }
  if (buffer_file_write (&vol->bf, &vol->count, sizeof (size_t)) == -1) {
    return -1;
  }
  if (buffer_file_seek (&vol->bf, position) == -1) {
    return -1;
  }

  return 0;
}

int
vga_op_list_write_assign (vga_op_list_t* vol,
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
  if (buffer_append (vol->bdb, bd) == -1) {
    return -1;
  }
  
  vga_op_type_t type = VGA_ASSIGN;
  if (buffer_file_write (&vol->bf, &type, sizeof (vga_op_type_t)) == -1 ||
      buffer_file_write (&vol->bf, &address, sizeof (size_t)) == -1 ||
      buffer_file_write (&vol->bf, &size, sizeof (size_t)) == -1 ||
      buffer_file_write (&vol->bf, &bd_offset, sizeof (size_t)) == -1) {
    return -1;
  }

  ++vol->count;

  size_t position = buffer_file_position (&vol->bf);

  if (buffer_file_seek (&vol->bf, 0) == -1) {
    return -1;
  }
  if (buffer_file_write (&vol->bf, &vol->count, sizeof (size_t)) == -1) {
    return -1;
  }
  if (buffer_file_seek (&vol->bf, position) == -1) {
    return -1;
  }

  return 0;
}

int
vga_op_list_initr (vga_op_list_t* vol,
		   bd_t bda,
		   bd_t bdb,
		   size_t* count)
{
  if (buffer_file_initr (&vol->bf, bda) == -1) {
    return -1;
  }

  if (buffer_file_read (&vol->bf, &vol->count, sizeof (size_t)) == -1) {
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

  if (buffer_file_read (&vol->bf, type, sizeof (vga_op_type_t)) == -1) {
    return -1;
  }

  /* Reset. */
  if (buffer_file_seek (&vol->bf, pos) == -1) {
    return -1;
  }

  return 0;
}

int
vga_op_list_read_set_start_address (vga_op_list_t* vol,
				    size_t* address)
{
  vga_op_type_t type;
  if (buffer_file_read (&vol->bf, &type, sizeof (vga_op_type_t)) == -1 ||
      type != VGA_SET_START_ADDRESS ||
      buffer_file_read (&vol->bf, address, sizeof (size_t)) == -1) {
    return -1;
  }

  return 0;
}

int
vga_op_list_read_set_cursor_location (vga_op_list_t* vol,
				      size_t* location)
{
  vga_op_type_t type;
  if (buffer_file_read (&vol->bf, &type, sizeof (vga_op_type_t)) == -1 ||
      type != VGA_SET_CURSOR_LOCATION ||
      buffer_file_read (&vol->bf, location, sizeof (size_t)) == -1) {
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
  size_t bd_offset;
  if (buffer_file_read (&vol->bf, &type, sizeof (vga_op_type_t)) == -1 ||
      type != VGA_ASSIGN ||
      buffer_file_read (&vol->bf, address, sizeof (size_t)) == -1 ||
      buffer_file_read (&vol->bf, size, sizeof (size_t)) == -1 ||
      buffer_file_read (&vol->bf, &bd_offset, sizeof (size_t)) == -1) {
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
