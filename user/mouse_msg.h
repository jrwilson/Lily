#ifndef MOUSE_MSG_H
#define MOUSE_MSG_H

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
} mouse_packet_list_t;

int
mouse_packet_list_initw (mouse_packet_list_t* vol,
			 bd_t bda);

int
mouse_packet_list_reset (mouse_packet_list_t* vol);

int
mouse_packet_list_write (mouse_packet_list_t* vol,
			 const mouse_packet_t *mp);

int
mouse_packet_list_initr (mouse_packet_list_t* vol,
			 bd_t bda,
			 size_t* count);

int
mouse_packet_list_read (mouse_packet_list_t* vol,
			mouse_packet_t *mp);

#endif /* MOUSE_MSG_H */
