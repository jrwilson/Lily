/*
  File
  ----
  frame.h
  
  Description
  -----------
  A frame is a page-sized unit of physical memory.

  Authors:
  Justin R. Wilson
*/

#include "frame.hpp"
#include "vm_manager.hpp"

frame::frame (const page_directory_entry& entry) :
  f_ (entry.frame_)
{ }

frame::frame (const page_table_entry& entry) :
  f_ (entry.frame_)
{ }
