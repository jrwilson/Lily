#ifndef VGA_MSG_H
#define VGA_MSG_H

#include <automaton.h>
#include <buffer_file.h>

#define VGA_VIDEO_MEMORY_BEGIN 0xA0000
#define VGA_VIDEO_MEMORY_END   0xC0000
#define VGA_VIDEO_MEMORY_SIZE (VGA_VIDEO_MEMORY_END - VGA_VIDEO_MEMORY_BEGIN)

#define VGA_TEXT_MEMORY_BEGIN 0xB8000
#define VGA_TEXT_MEMORY_END   0xC0000
#define VGA_TEXT_MEMORY_SIZE  (VGA_TEXT_MEMORY_END - VGA_TEXT_MEMORY_BEGIN)

typedef enum {
  VGA_SET_CURSOR_LOCATION,
  VGA_ASSIGN,
  VGA_BASSIGN,
} vga_op_type_t;

typedef struct {
  buffer_file_t bf;
  bd_t bdb;
  size_t count;
  size_t bdb_size;
  const void* ptr;
} vga_op_list_t;

int
vga_op_list_initw (vga_op_list_t* vol,
		   bd_t bda,
		   bd_t bdb);

void
vga_op_list_reset (vga_op_list_t* vol);

int
vga_op_list_write_set_cursor_location (vga_op_list_t* vol,
				       size_t location);

int
vga_op_list_write_assign (vga_op_list_t* vol,
			  size_t address,
			  const void* data,
			  size_t size);

int
vga_op_list_write_bassign (vga_op_list_t* vol,
			   size_t address,
			   size_t size,
			   bd_t bd);

int
vga_op_list_initr (vga_op_list_t* vol,
		   bd_t bda,
		   bd_t bdb,
		   size_t* count);

int
vga_op_list_next_op_type (vga_op_list_t* vol,
			  vga_op_type_t* type);

int
vga_op_list_read_set_cursor_location (vga_op_list_t* vol,
				      size_t* location);

int
vga_op_list_read_assign (vga_op_list_t* vol,
			 size_t* address,
			 const void** data,
			 size_t* size);

int
vga_op_list_read_bassign (vga_op_list_t* vol,
			  size_t* address,
			  const void** data,
			  size_t* size);

#endif /* VGA_MSG_H */
