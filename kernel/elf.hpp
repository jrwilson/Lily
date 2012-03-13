#ifndef __elf_hpp__
#define __elf_hpp__

#include "integer_types.hpp"
#include "buffer_file.hpp"
#include "kstring.hpp"

// I stole this from Linkers and Loaders (John R. Levine, p. 64).
namespace elf {    
  static const char MAGIC[4] = { '\177', 'E', 'L', 'F' };

  enum width {
    BIT32 = 1,
    BIT64 = 2,
  };

  enum byte_order {
    LITTLE_END = 1,
    BIG_END = 2,
  };

  struct preamble {
    char magic[4];
    char width;			// See width above.
    char byte_order;		// See byte_order above.
    char header_version;	// Header version.
    char pad[9];
  };

  enum filetype {
    RELOCATABLE = 1,
    EXECUTABLE = 2,
    SHARED_OBJECT = 3,
    CORE_IMAGE = 4
  };

  enum archtype {
    SPARC = 2,
    X86 = 3,
    M68K = 4,
  };

  struct header {
    uint16_t filetype;			// See filtype above.
    uint16_t archtype;			// See archtype above.
    uint32_t file_version;		// File version (always 1).
    uint32_t entry_point;		// Entry point.
    uint32_t program_header_offset;	// Offset of the program header.
    uint32_t section_header_offset;	// Offset of the section header.
    uint32_t flags;			// Architecture specific flags.
    uint16_t header_size;		// Size of this header.
    uint16_t program_header_entry_size;	// Size of an entry in the program header.
    uint16_t program_header_entry_count;// Number of entries in the program header.
    uint16_t section_header_entry_size;	// Size of an entry in the section header.
    uint16_t section_header_entry_count;// Number of entries in the section header.
    uint16_t string_section;		// Section number that contains section name strings.
  };

  enum segment_type {
    NULL_ENTRY = 0,
    LOAD = 1,
    DYNAMIC = 2,
    INTERP = 3,
    NOTE = 4,
    SHLIB = 5,
    PHDR = 6,
  };

  enum permission {
    EXECUTE = 1,
    WRITE = 2,
    READ = 4,
  };

  struct program_header_entry {
    uint32_t type;		// See segment_type above.
    uint32_t offset;		// File offset of segment.
    uint32_t virtual_address;	// Virtual address of segment.
    uint32_t physical_address;	// Physical address of segment.
    uint32_t file_size;		// Size of the segment in the file.
    uint32_t memory_size;	// Size of the segment in memory.
    uint32_t permissions;	// Read, write, execute bits.  See permission above.
    uint32_t alignment;		// Required alignment.
  };

  struct note {
    uint32_t name_size;
    uint32_t desc_size;
    uint32_t type;
  };

  enum note_type {
    ACTION_DESCRIPTOR = 0,
  };

  struct action_descriptor {
    uint32_t action_type;
    uint32_t parameter_mode;
    uint32_t action_entry_point;
    uint32_t action_number;
    uint32_t action_name_size;
    uint32_t action_description_size;
  };

  typedef vector<program_header_entry> program_header_list_type;
  typedef unordered_map<logical_address_t, pair<frame_t, vm::map_mode_t> > frame_map_type;
  typedef vector<pair<logical_address_t, logical_address_t> > clear_list_type;

  // Interpret a region of memory as an ELF file.
  int
  parse (const shared_ptr<automaton>& a,
	 logical_address_t begin,
	 logical_address_t end)
  {
    buffer_file bf (begin, end);

    // Read the preamble.
    const preamble* const preamble_ = static_cast<const preamble*> (bf.readp (sizeof (preamble)));
    if (preamble_ == 0) {
      return -1;
    }

    // Check the magic string.
    if (memcmp (preamble_->magic, MAGIC, sizeof (MAGIC)) != 0) {
      return -1;
    }
    
    // Check the width.
    if (preamble_->width != BIT32) {
      // TODO:  Support 64-bit.
      return -1;
    }

    if (preamble_->byte_order != LITTLE_END) {
      // TODO:  Support big-endian.
      return -1;
    }
    
    if (preamble_->header_version != 1) {
      // TODO:  Support other versions.
      return -1;
    }

    // Read the header.
    const header* const header_ = static_cast<const header*> (bf.readp (sizeof (header)));
    if (header_ == 0) {
      return -1;
    }
    
    if (header_->filetype != EXECUTABLE) {
      // TODO:  We only support executable_files.
      return -1;
    }

    if (header_->archtype != X86) {
      // TODO:  We only support x86.
      return -1;
    }
    
    if (header_->file_version != 1) {
      // TODO:  Support other version.
      return -1;
    }
    
    // Ignore the entry point.
    
    if (header_->program_header_offset == 0) {
      // File doesn't contain a program header.
      return -1;
    }
    
    // Ignore the section header offset.
    
    // Ignore the flags.
    
    if (header_->header_size != sizeof (preamble) + sizeof (header)) {
      // Safety check (as far as I can tell).
      return -1;
    }
    
    if (header_->program_header_entry_size != sizeof (program_header_entry)) {
      // The size of the program header entries is incorrect.
      return -1;
    }
    
    if (header_->program_header_entry_count == 0) {
      // Must have at least one program entry.
      return -1;
    }
    
    // Ignore the section headers.
    
    // Ignore the section number containing the name strings.
    
    
    // Check the program header.
    
    // Seek to the program header.
    bf.seek (header_->program_header_offset);

    // A list of frames used when creating automata.
    frame_map_type frame_map_;
    // A list of frames to clear.
    clear_list_type clear_list_;

    for (size_t idx = 0; idx != header_->program_header_entry_count; ++idx) {
      // Read the program header entries.
      const program_header_entry* const e = static_cast<const program_header_entry*> (bf.readp (sizeof (program_header_entry)));
      if (e == 0) {
	return -1;
      }

      switch (e->type) {
      case NULL_ENTRY:
	// Do nothing.
	break;
      case LOAD:
	if (e->memory_size != 0) {

	  // Record our position.
	  size_t pos = bf.position ();
	  
	  // Seek to the section and try to read it.
	  bf.seek (e->offset);
	  if (bf.readp (e->file_size) == 0) {
	    return -1;
	  }
	  
	  // Restore our position.
	  bf.seek (pos);
	  
	  if (e->virtual_address >= KERNEL_VIRTUAL_BASE) {
	    // Interferes with kernel.
	    return -1;
	  }
	  if (e->virtual_address + e->memory_size > KERNEL_VIRTUAL_BASE) {
	    // Interferes with kernel.
	    return -1;
	  }
	  if (e->file_size > e->memory_size) {
	    // Extra data in file.
	    return -1;
	  }
	  if (e->alignment != PAGE_SIZE) {
	    // Unsupported alignment.
	    return -1;
	  }
	  if ((e->offset & (e->alignment - 1)) != (e->virtual_address & (e->alignment - 1))) {
	    // Not aligned in the file.
	    return -1;
	  }
	  if ((e->permissions & (elf::EXECUTE | elf::WRITE | elf::READ)) == 0) {
	    // No permissions.
	    return -1;
	  }

	  // BUG:  Check the memory map from the parse.
  
	  vm::map_mode_t map_mode = ((e->permissions & elf::WRITE) != 0) ? vm::MAP_COPY_ON_WRITE : vm::MAP_READ_ONLY;
	  
	  size_t s;
	  // Initialized data.
	  for (s = 0; s < e->file_size; s += PAGE_SIZE) {
	    pair<frame_map_type::iterator, bool> r = frame_map_.insert (make_pair (align_down (e->virtual_address + s, PAGE_SIZE), make_pair (vm::logical_address_to_frame (begin + e->offset + s), map_mode)));
	    if (!r.second && map_mode == vm::MAP_COPY_ON_WRITE) {
	      // Already existed and needs to be copy-on-write.
	      r.first->second.second = vm::MAP_COPY_ON_WRITE;
	    }
	  }
	  
	  // Clear the tiny region between the end of initialized data and the first unitialized page.
	  if (e->file_size < e->memory_size) {
	    logical_address_t begin = e->virtual_address + e->file_size;
	    logical_address_t end = e->virtual_address + e->memory_size;
	    clear_list_.push_back (make_pair (begin, end));
	  }
	  
	  // Uninitialized data.
	  for (; s < e->memory_size; s += PAGE_SIZE) {
	    pair<frame_map_type::iterator, bool> r = frame_map_.insert (make_pair (align_down (e->virtual_address + s, PAGE_SIZE), make_pair (vm::zero_frame (), map_mode)));
	    if (!r.second && map_mode == vm::MAP_COPY_ON_WRITE) {
	      // Already existed and needs to be copy-on-write.
	      r.first->second.second = vm::MAP_COPY_ON_WRITE;
	    }
	  }
	  
	  if (!a->vm_area_is_free (e->virtual_address, e->virtual_address + e->memory_size)) {
	    // The area conflicts with the existing memory map.
	    return -1;
	  }
	  
	  a->insert_vm_area (new vm_area_base (e->virtual_address, e->virtual_address + e->memory_size));

	  program_header_list_type program_headers_;
	  
	  // Record the header.
	  program_headers_.push_back (*e);
	}
	break;
      case DYNAMIC:
	// Don't know how to parse dynamic sections.
	break;
      case INTERP:
	// Don't know how to parse interpretted sections.
	break;
      case NOTE:
	if (e->file_size != 0) {
	  
	  if (e->alignment != 4) {
	    // Must be aligned to 32-bit integers.
	    return -1;
	  }
	  
	  // Record our position.
	  size_t pos = bf.position ();
	  
	  // Seek to the section and try to read it.
	  bf.seek (e->offset);
	  const logical_address_t note_begin = reinterpret_cast<logical_address_t> (bf.readp (0));
	  if (bf.readp (e->file_size) == 0) {
	    return -1;
	  }
	  
	  // Restore our position.
	  bf.seek (pos);
	  
	  // We'll treat the note section as a file within a file.
	  buffer_file note_bf (note_begin, note_begin + e->file_size);
	  
	  const note* n;
	  for (;;) {
	    n = static_cast<const note*> (note_bf.readp (sizeof (note)));
	    if (n != 0) {
	      const char* name = static_cast<const char*> (note_bf.readp (align_up (n->name_size, 4)));
	      if (name == 0) {
		return -1;
	      }
	      if (name[n->name_size - 1] != 0) {
		// Name is not null-terminated.
		return -1;
	      }
	      
	      const logical_address_t desc = reinterpret_cast<logical_address_t> (note_bf.readp (align_up (n->desc_size, 4)));
	      if (desc == 0) {
		return -1;
	      }
	      
	      if (strcmp (name, "lily") == 0) {
		// Note is intended for us.
		switch (n->type) {
		case ACTION_DESCRIPTOR:
		  {
		    // Parse the description like a file.
		    buffer_file desc_bf (desc, desc + n->desc_size);
		    
		    const action_descriptor* d = static_cast<const action_descriptor*> (desc_bf.readp (sizeof (action_descriptor)));
		    if (d == 0) {
		      return -1;
		    }
		    
		    switch (d->action_type) {
		    case INPUT:
		    case OUTPUT:
		    case INTERNAL:
		    case SYSTEM_INPUT:
		      break;
		    default:
		      // Unknown action type.
		      return -1;
		    }
		    
		    switch (d->parameter_mode) {
		    case NO_PARAMETER:
		    case PARAMETER:
		    case AUTO_PARAMETER:
		      break;
		    default:
		      // Unknown parameter mode.
		      return -1;
		    }
		    
		    const char* action_name = static_cast<const char*> (desc_bf.readp (d->action_name_size));
		    if (action_name == 0) {
		      return -1;
		    }
		    if (action_name[d->action_name_size - 1] != 0) {
		      return -1;
		    }
		    
		    const char* action_description = static_cast<const char*> (desc_bf.readp (d->action_description_size));
		    if (action_description == 0) {
		      return -1;
		    }
		    if (action_description[d->action_description_size - 1] != 0) {
		      return -1;
		    }
		    
		    switch (d->action_number) {
		    case LILY_ACTION_NO_ACTION:
		      // The action number LILY_ACTION_NO_ACTION is reserved.
		      return -1;
		      break;
		    case LILY_ACTION_INIT:
		      if (d->action_type != LILY_ACTION_SYSTEM_INPUT || d->parameter_mode != PARAMETER) {
			// These must have the correct type.
			return -1;
		      }
		      // Fall through.
		    default:
		      if (!a->add_action (static_cast<action_type_t> (d->action_type), static_cast<parameter_mode_t> (d->parameter_mode), reinterpret_cast<const void*> (d->action_entry_point), d->action_number, kstring (action_name, d->action_name_size), kstring (action_description, d->action_description_size))) {
			// Action conflicts.
			return -1;
		      }
		      break;
		    }

		  }
		  break;
		default:
		  // Unknown note.
		  return -1;
		}	  
	      }
	    }
	    else {
	      break;
	    }
	  }	      
	}
	break;
      case SHLIB:
	// Don't know how to parse shared library sections.
	break;
      case PHDR:
	// Don't know how to parse program header sections ;)
	break;
      }
    }

    if (!a->insert_heap_and_stack ()) {
      // Memory map interfers with heap and stack.
      return -1;
    }

    // Switch to the automaton.
    physical_address_t old = vm::switch_to_directory (a->page_directory);
    
    // We can only use data in the kernel, i.e., we can't use automaton_text or hp.
    
    // Map all the frames.
    for (frame_map_type::const_iterator pos = frame_map_.begin ();
	 pos != frame_map_.end ();
	 ++pos) {
      vm::map (pos->first, pos->second.first, vm::USER, pos->second.second);
    }
    
    // Clear.
    for (clear_list_type::const_iterator pos = clear_list_.begin ();
	 pos != clear_list_.end ();
	 ++pos) {
      memset (reinterpret_cast<void*> (pos->first), 0, pos->second - pos->first);
    }
    
    // Map the heap and stack while using the automaton's page directory.
    a->map_heap_and_stack ();
    
    // Switch back.
    vm::switch_to_directory (old);
      
    return 0;
  }
}

#endif
