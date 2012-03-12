#include "ps2_mouse_msg.h"

int
ps2_mouse_packet_list_initw (ps2_mouse_packet_list_t* vol,
			     bd_t bda)
{
  if (buffer_file_initw (&vol->bf, bda) == -1) {
    return -1;
  }

  vol->count = 0;

  if (buffer_file_seek (&vol->bf, sizeof (size_t)) == -1) {
    return -1;
  }

  return 0;
}

int
ps2_mouse_packet_list_reset (ps2_mouse_packet_list_t* vol)
{
  if (vol->count == 0) {
    return 0;
  }

  vol->count = 0;

  buffer_file_truncate (&vol->bf);

  if (buffer_file_seek (&vol->bf, sizeof (size_t)) == -1) {
    return -1;
  }

  return 0;
}

int
ps2_mouse_packet_list_write (ps2_mouse_packet_list_t* vol,
			     const mouse_packet_t *mp)
{
  if (buffer_file_write (&vol->bf, mp, sizeof (mouse_packet_t)) == -1) {
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
ps2_mouse_packet_list_initr (ps2_mouse_packet_list_t* vol,
			     bd_t bda,
			     size_t* count)
{
  if (buffer_file_initr (&vol->bf, bda) == -1) {
    return -1;
  }

  if (buffer_file_read (&vol->bf, &vol->count, sizeof (size_t)) == -1) {
    return -1;
  }

  *count = vol->count;

  return 0;
}

int
ps2_mouse_packet_list_read (ps2_mouse_packet_list_t* vol,
			    mouse_packet_t *mp)
{
  if (buffer_file_read (&vol->bf, mp, sizeof (mouse_packet_t)) == -1) {
    return -1;
  }

  return 0;
}


