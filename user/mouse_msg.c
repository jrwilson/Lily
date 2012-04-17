#include "mouse_msg.h"

int
mouse_packet_list_initw (mouse_packet_list_t* vol,
			 lily_error_t* err,
			 bd_t bda)
{
  if (buffer_file_initw (&vol->bf, err, bda) != 0) {
    return -1;
  }
  
  vol->count = 0;
  
  if (buffer_file_seek (&vol->bf, sizeof (size_t)) != 0) {
    return -1;
  }
  
  return 0;
}

int
mouse_packet_list_reset (mouse_packet_list_t* vol)
{
  if (vol->count == 0) {
    return 0;
  }
  
  vol->count = 0;
  
  buffer_file_truncate (&vol->bf);
  
  if (buffer_file_seek (&vol->bf, sizeof (size_t)) != 0) {
    return -1;
  }
  
  return 0;
}

int
mouse_packet_list_write (mouse_packet_list_t* vol,
			 lily_error_t* err,
			 const mouse_packet_t *mp)
{
  if (buffer_file_write (&vol->bf, err, mp, sizeof (mouse_packet_t)) != 0) {
    return -1;
  }
  
  ++vol->count;
  
  size_t position = buffer_file_position (&vol->bf);
  
  if (buffer_file_seek (&vol->bf, 0) != 0) {
    return -1;
  }
  if (buffer_file_write (&vol->bf, err, &vol->count, sizeof (size_t)) != 0) {
    return -1;
  }
  if (buffer_file_seek (&vol->bf, position) != 0) {
    return -1;
  }
  
  return 0;
}

int
mouse_packet_list_initr (mouse_packet_list_t* vol,
			 lily_error_t* err,
			 bd_t bda)
{
  if (buffer_file_initr (&vol->bf, err, bda) != 0) {
    return -1;
  }
  
  if (buffer_file_read (&vol->bf, &vol->count, sizeof (size_t)) != 0) {
    return -1;
  }
  
  return 0;
}

int
mouse_packet_list_read (mouse_packet_list_t* vol,
			mouse_packet_t *mp)
{
  if (buffer_file_read (&vol->bf, mp, sizeof (mouse_packet_t)) != 0) {
    return -1;
  }
  
  return 0;
}
