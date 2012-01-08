#ifndef __elf_hpp__
#define __elf_hpp__

// I stole this from Linkers and Loaders (John R. Levine, p. 64).
namespace elf {    
  // struct note {
  //   uint32_t name_size;
  //   uint32_t desc_size;
  //   uint32_t type;

  //   const char*
  //   name () const
  //   {
  //     if (name_size != 0) {
  // 	return reinterpret_cast<const char*> (&type + 1);
  //     }
  //     else {
  // 	return 0;
  //     }
  //   }

  //   const void*
  //   desc () const
  //   {
  //     return name () + align_up (name_size, 4);
  //   }

  //   const note*
  //   next () const
  //   {
  //     return reinterpret_cast<const elf::note*> (reinterpret_cast<const uint8_t*> (this) + sizeof (elf::note) + align_up (name_size, 4) + align_up (desc_size, 4));
  //   }
  // };

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

  // Interpret a region of memory as an ELF header.
  class header_parser {
  public:
    header_parser (const void* ptr,
		   size_t size) :
      preamble_ (static_cast<const preamble*> (ptr)),
      end_ (static_cast<const uint8_t*> (ptr) + size)
    { }
    
    bool
    parse (void)
    {
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
      program_header_begin_ = reinterpret_cast<const program_header_entry*> (reinterpret_cast<const uint8_t*> (preamble_) + header_->program_header_offset);

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
      program_header_end_ = program_header_begin_ + header_->program_header_entry_count;

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

    const program_header_entry*
    program_header_begin (void) const
    {
      return program_header_begin_;
    }

    const program_header_entry*
    program_header_end (void) const
    {
      return program_header_end_;
    }

  private:
    const preamble* const preamble_;
    const void* const end_;
    const program_header_entry* program_header_begin_;
    const program_header_entry* program_header_end_;
  };

}

#endif
