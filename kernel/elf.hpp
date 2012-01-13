#ifndef __elf_hpp__
#define __elf_hpp__

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

    const char*
    name () const
    {
      return reinterpret_cast<const char*> (&type + 1);
    }

    const void*
    desc () const
    {
      return name () + align_up (name_size, 4);
    }

    const note*
    next () const
    {
      return reinterpret_cast<const elf::note*> (reinterpret_cast<const uint8_t*> (this) + sizeof (elf::note) + align_up (name_size, 4) + align_up (desc_size, 4));
    }
  };

  enum note_type {
    ACTION_DESCRIPTOR = 0,
  };

  struct action_descriptor {
    uint32_t name_size;
    uint32_t desc_size;
    uint32_t compare_method;
    uint32_t action_type;
    uint32_t action_entry_point;
    uint32_t parameter_mode;

    const char*
    name () const
    {
      return reinterpret_cast<const char*> (this + 1);
    }

    const char*
    desc () const
    {
      return name () + name_size;
    }
  };

  // Interpret a region of memory as an ELF file.
  class parser {
  public:
    bool
    parse (automaton* a,
	   logical_address_t begin,
	   logical_address_t end)
    {
      const preamble* const preamble_ = reinterpret_cast<const preamble*> (begin);
      const void* const end_ = reinterpret_cast<const void*> (end);

      // Ensure we have enough data to process.
      if (preamble_ + 1 > end_) {
	return false;
      }

      // Check the magic string.
      if (memcmp (preamble_->magic, MAGIC, sizeof (MAGIC)) != 0) {
	return false;
      }

      // Check the width.
      if (preamble_->width != BIT32) {
	// TODO:  Support 64-bit.
	return false;
      }

      if (preamble_->byte_order != LITTLE_END) {
	// TODO:  Support big-endian.
	return false;
      }

      if (preamble_->header_version != 1) {
	// TODO:  Support other versions.
	return false;
      }

      // Compute the location of the header.
      const header* header_ = reinterpret_cast<const header*> (preamble_ + 1);

      // Ensure we have enough data to process.
      if (header_ + 1 > end_) {
	return false;
      }

      if (header_->filetype != EXECUTABLE) {
	// TODO:  We only support executable_files.
	return false;
      }

      if (header_->archtype != X86) {
	// TODO:  We only support x86.
	return false;
      }

      if (header_->file_version != 1) {
	// TODO:  Support other version.
	return false;
      }

      // Ignore the entry point.

      if (header_->program_header_offset == 0) {
	// File doesn't contain a program header.
	return false;
      }

      // Compute the location of the program header.
      const program_header_entry* program_header_begin_ = reinterpret_cast<const program_header_entry*> (reinterpret_cast<const uint8_t*> (preamble_) + header_->program_header_offset);

      // Ignore the section header offset.

      // Ignore the flags.

      if (header_->header_size != sizeof (preamble) + sizeof (header)) {
	// Safety check (as far as I can tell).
	return false;
      }

      if (header_->program_header_entry_size != sizeof (program_header_entry)) {
	// The size of the program header entries is incorrect.
	return false;
      }

      if (header_->program_header_entry_count == 0) {
	// Must have at least one program entry.
	return false;
      }

      // Calculate the end of the program header.
      const program_header_entry* program_header_end_ = program_header_begin_ + header_->program_header_entry_count;

      // Both the beginning and end must be range.
      if (program_header_begin_ + 1 > end_) {
	return false;
      }
      if (program_header_end_ > end_) {
	return false;
      }
      if (program_header_begin_ >= program_header_end_) {
	return false;
      }

      // Ignore the section headers.

      // Ignore the section number containing the name strings.

      // Check the program header.
      for (const program_header_entry* e = program_header_begin_;
	   e != program_header_end_;
	   ++e) {

	switch (e->type) {
	case NULL_ENTRY:
	  // Do nothing.
	  break;
	case LOAD:
	  if (e->memory_size != 0) {
	    if (reinterpret_cast<const int8_t*> (preamble_) + e->offset >= end_) {
	      // Section is not in file.
	      return false;
	    }
	    if (reinterpret_cast<const int8_t*> (preamble_) + e->offset + e->file_size > end_) {
	      // Section ends outside of file.
	      return false;
	    }
	    if (e->virtual_address >= KERNEL_VIRTUAL_BASE) {
	      // Interferes with kernel.
	      return false;
	    }
	    if (e->virtual_address + e->memory_size > KERNEL_VIRTUAL_BASE) {
	      // Interferes with kernel.
	      return false;
	    }
	    if (e->file_size > e->memory_size) {
	      // Extra data in file.
	      return false;
	    }
	    if (e->alignment != PAGE_SIZE) {
	      // Unsupported alignment.
	      return false;
	    }
	    if ((e->offset & (e->alignment - 1)) != (e->virtual_address & (e->alignment - 1))) {
	      // Not aligned in the file.
	      return false;
	    }
	    if ((e->permissions & (elf::EXECUTE | elf::WRITE | elf::READ)) == 0) {
	      // No permissions.
	      return false;
	    }
	    
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
	    if (reinterpret_cast<const int8_t*> (preamble_) + e->offset >= end_) {
	      // Section is not in file.
	      return false;
	    }
	    if (reinterpret_cast<const int8_t*> (preamble_) + e->offset + e->file_size > end_) {
	      // Section ends outside of file.
	      return false;
	    }
	    if (e->alignment != 4) {
	      // Must be aligned to 32-bit integers.
	      return false;
	    }

	    // Parse the notes.
	    for (const note* n = reinterpret_cast<const note*> (reinterpret_cast<const int8_t*> (preamble_) + e->offset);
		 n < static_cast<const void*> (reinterpret_cast<const int8_t*> (preamble_) + e->offset + e->file_size);
		 n = n->next ()) {

	      if (n + 1 > end_) {
		// Note is not in file.
		return false;
	      }
	      
	      if (n->name () > end_) {
		// Name is not in file.
		return false;
	      }
	      
	      if (n->name () + n->name_size > end_) {
		// Name is not in file.
		return false;
	      }
	      
	      if (n->name ()[n->name_size - 1] != 0) {
		// Name is not null-terminated.
		return false;
	      }
	      
	      if (n->desc () > end_) {
		// Description is not in file.
		return false;
	      }
	      
	      if (static_cast<const uint8_t*> (n->desc ()) + n->desc_size > end_) {
		// Description is not in file.
		return false;
	      }
	      
	      if (strcmp (n->name (), "lily") == 0) {
		// Note is intended for us.
		switch (n->type) {
		case ACTION_DESCRIPTOR:
		  {
		    const action_descriptor* d = static_cast<const action_descriptor*> (n->desc ());
		    if (d + 1 > static_cast<const void*> (n->next ())) {
		      // Action description is not in file.
		      return false;
		    }
		    
		    if (d->name () > static_cast<const void*> (n->next ())) {
		      return false;
		    }
		    
		    if (d->name () + d->name_size > static_cast<const void*> (n->next ())) {
		      return false;
		    }
		    
		    if (d->name ()[d->name_size - 1] != 0) {
		      return false;
		    }
		    
		    if (d->desc () > static_cast<const void*> (n->next ())) {
		      return false;
		    }
		    
		    if (d->desc () + d->desc_size > static_cast<const void*> (n->next ())) {
		      return false;
		    }
		    
		    if (d->desc ()[d->desc_size - 1] != 0) {
		      return false;
		    }
		    
		    switch (d->compare_method) {
		    case NO_COMPARE:
		    case EQUAL:
		      break;
		    default:
		      // Unknown method.
		      return false;
		    }
		    
		    switch (d->action_type) {
		    case INPUT:
		    case OUTPUT:
		    case INTERNAL:
		      break;
		    default:
		      // Unknown action type.
		      return false;
		    }
		    
		    switch (d->parameter_mode) {
		    case NO_PARAMETER:
		    case PARAMETER:
		    case AUTO_PARAMETER:
		      break;
		    default:
		      // Unknown parameter mode.
		      return false;
		    }

		    actions_.push_back (new paction (a, d->name (), d->desc (), static_cast<compare_method_t> (d->compare_method), static_cast<action_type_t> (d->action_type), reinterpret_cast<const void*> (d->action_entry_point), static_cast<parameter_mode_t> (d->parameter_mode)));
		  }
		  break;
		default:
		  // Unknown note.
		  return false;
		}
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

      return true;
    }

    typedef std::vector<program_header_entry> program_header_list_type;
    typedef program_header_list_type::const_iterator program_header_iterator;

    program_header_iterator
    program_header_begin () const
    {
      return program_headers_.begin ();
    }

    program_header_iterator
    program_header_end () const
    {
      return program_headers_.end ();
    }

    typedef std::vector<paction*> action_list_type;
    typedef action_list_type::const_iterator action_iterator;

    action_iterator
    action_begin () const
    {
      return actions_.begin ();
    }

    action_iterator
    action_end () const
    {
      return actions_.end ();
    }

  private:
    program_header_list_type program_headers_;
    action_list_type actions_;
  };

}

#endif
