#ifndef PS2_MOUSE_MSG_H
#define PS2_MOUSE_MSG_H

#include <automaton.h>
#include <buffer_file.h>

typedef struct {
  unsigned int button_status_bits;
  int x_delta;
  int y_delta;
  int z_delta_vertical;
  int z_delta_horizontal;
  mono_time_t time_stamp;
} mouse_packet_t;

typedef struct {
  buffer_file_t bf; // buffer file abstraction (over a buffer)
  size_t count;     // count of enqueued mouse packets
} ps2_mouse_packet_list_t;

int
ps2_mouse_packet_list_initw (ps2_mouse_packet_list_t* vol,
			     bd_t bda);

int
ps2_mouse_packet_list_reset (ps2_mouse_packet_list_t* vol);

int
ps2_mouse_packet_list_write (ps2_mouse_packet_list_t* vol,
			     const mouse_packet_t *mp);

int
ps2_mouse_packet_list_initr (ps2_mouse_packet_list_t* vol,
			     bd_t bda,
			     size_t* count);

int
ps2_mouse_packet_list_read (ps2_mouse_packet_list_t* vol,
			    mouse_packet_t *mp);

#endif /* PS2_MOUSE_MSG_H */
